/*********************                                                        */
/*! \file
** \verbatim
** Top contributors (to current version):
**   Makai Mann, Ahmed Irfan
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief
**
**
**/

#include <csignal>
#include <iostream>
#include "assert.h"

#ifdef WITH_PROFILING
#include <gperftools/profiler.h>
#endif

#include "smt-switch/boolector_factory.h"
#ifdef WITH_MSAT
#include "smt-switch/msat_factory.h"
#endif

#include "core/fts.h"
#include "core/tts.h"
#include "frontends/btor2_encoder.h"
#include "frontends/smv_encoder.h"
#include "frontends/vmt_encoder.h"
#include "frontends/timed_vmt_encoder.h"
#include "modifiers/control_signals.h"
#include "modifiers/mod_ts_prop.h"
#include "modifiers/prop_monitor.h"
#include "modifiers/static_coi.h"
#include "modifiers/property_rtc.h"
#include "options/options.h"
#include "printers/btor2_witness_printer.h"
#include "printers/vcd_witness_printer.h"
#include "smt-switch/logging_solver.h"
#include "smt/available_solvers.h"
#include "utils/logger.h"
#include "utils/timestamp.h"
#include "utils/make_provers.h"
#include "utils/ts_analysis.h"
#include "engines/ic3ia.h"
#include "engines/ic3iartc.h"

using namespace pono;
using namespace smt;
using namespace std;


ProverResult check_prop(PonoOptions pono_options,
                        Term & prop,
                        TransitionSystem & ts,
                        const SmtSolver & s,
                        std::vector<UnorderedTermMap> & cex)
{
  // get property name before it is rewritten
  const string prop_name = ts.get_name(prop);

  logger.log(1, "Solving property: {}", prop_name);
  logger.log(3, "INIT:\n{}", ts.init());
  logger.log(3, "TRANS:\n{}", ts.trans());
  logger.log(3, "PROP:\n{}", prop);

  // modify the transition system and property based on options
  if (!pono_options.clock_name_.empty()) {
    Term clock_symbol = ts.lookup(pono_options.clock_name_);
    toggle_clock(ts, clock_symbol);
  }
  if (!pono_options.reset_name_.empty()) {
    std::string reset_name = pono_options.reset_name_;
    bool negative_reset = false;
    if (reset_name.at(0) == '~') {
      reset_name = reset_name.substr(1, reset_name.length() - 1);
      negative_reset = true;
    }
    Term reset_symbol = ts.lookup(reset_name);
    if (negative_reset) {
      SortKind sk = reset_symbol->get_sort()->get_sort_kind();
      reset_symbol = (sk == BV) ? s->make_term(BVNot, reset_symbol)
                                : s->make_term(Not, reset_symbol);
    }
    Term reset_done = add_reset_seq(ts, reset_symbol, pono_options.reset_bnd_);
    // guard the property with reset_done
    prop = ts.solver()->make_term(Implies, reset_done, prop);
  }


  if (pono_options.static_coi_) {
    /* Compute the set of state/input variables related to the
       bad-state property. Based on that information, rebuild the
       transition relation of the transition system. */
    StaticConeOfInfluence coi(ts, { prop }, pono_options.verbosity_);
  }

  if (pono_options.pseudo_init_prop_) {
    ts = pseudo_init_and_prop(ts, prop);
  }

  if (pono_options.promote_inputvars_) {
    ts = promote_inputvars(ts);
    assert(!ts.inputvars().size());
  }

  if (!ts.only_curr(prop)) {
    logger.log(1,
               "Got next state or input variables in property. "
               "Generating a monitor state.");
    prop = add_prop_monitor(ts, prop);
  }

  if (pono_options.assume_prop_) {
    // NOTE: crucial that pseudo_init_prop and add_prop_monitor passes are
    // before this pass. Can't assume the non-delayed prop and also
    // delay it
    prop_in_trans(ts, prop);
  }

  Property p(s, prop, prop_name);

  // If we are checking rt-consistency, overwrite the property to the quantified rtc formula
  if ( pono_options.rt_consistency_){
    if (ts.solver()->get_solver_enum() != CVC5){
      throw PonoException("RT-Consistency check is currently only supported with the smt solver CVC5");
    }
    logger.log(1, "Checking rt-consistency with mode {}", pono_options.rt_consistency_mode_);
    p = get_rt_consistency_property(p, ts, pono_options.rt_consistency_mode_);
    logger.log(3, "Quantified property: {} ", p.prop());
  }

  // end modification of the transition system and property

  Engine eng = pono_options.engine_;

  std::shared_ptr<Prover> prover;
  if (pono_options.cegp_abs_vals_) {
    prover = make_cegar_values_prover(eng, p, ts, s, pono_options);
  } else if (pono_options.ceg_bv_arith_) {
    prover = make_cegar_bv_arith_prover(eng, p, ts, s, pono_options);
  } else if (pono_options.ceg_prophecy_arrays_) {
    prover = make_ceg_proph_prover(eng, p, ts, s, pono_options);
  } else {
    prover = make_prover(eng, p, ts, s, pono_options);
  }

  assert(prover);

  // TODO: handle this in a more elegant way in the future
  //       consider calling prover for CegProphecyArrays (so that underlying
  //       model checker runs prove unbounded) or possibly, have a command line
  //       flag to pick between the two
  ProverResult r;
  if (pono_options.engine_ == MSAT_IC3IA)
  {
    // HACK MSAT_IC3IA does not support check_until
    r = prover->prove();
  }
  else
  {
    r = prover->check_until(pono_options.bound_);
  }

  if (r == FALSE && pono_options.witness_) {
    bool success = prover->witness(cex);
    if (!success) {
      logger.log(
          0,
          "Only got a partial witness from engine. Not suitable for printing.");
    }
  }

  Term invar;
  if (r == TRUE && (pono_options.show_invar_ || pono_options.check_invar_)) {
    try {
      invar = prover->invar();
    }
    catch (PonoException & e) {
      std::cout << "Engine " << pono_options.engine_
                << " does not support getting the invariant." << std::endl;
    }
  }

  if (r == TRUE && pono_options.show_invar_ && invar) {
    logger.log(0, "INVAR: {}", invar);
  }

  if (r == TRUE && pono_options.check_invar_ && invar) {
    bool invar_passes = check_invar(ts, p.prop(), invar);
    std::cout << "Invariant Check " << (invar_passes ? "PASSED" : "FAILED")
              << std::endl;
    if (!invar_passes) {
      // shouldn't return true if invariant is incorrect
      throw PonoException("Invariant Check FAILED");
    }
  }
  return r;
}

// Note: signal handlers are registered only when profiling is enabled.
void profiling_sig_handler(int sig)
{
  std::string signame;
  switch (sig) {
    case SIGINT: signame = "SIGINT"; break;
    case SIGTERM: signame = "SIGTERM"; break;
    case SIGALRM: signame = "SIGALRM"; break;
    default:
      throw PonoException(
          "Caught unexpected signal"
          "in profiling signal handler.");
  }
  logger.log(0, "\n Signal {} received\n", signame);
#ifdef WITH_PROFILING
  ProfilerFlush();
  ProfilerStop();
#endif
  // Switch back to default handling for signal 'sig' and raise it.
  signal(sig, SIG_DFL);
  raise(sig);
}

int main(int argc, char ** argv)
{
  auto begin_time_stamp = timestamp();

  PonoOptions pono_options;
  ProverResult res = pono_options.parse_and_set_options(argc, argv);
  if (res == ERROR) return res;
  // expected result returned by option parsing and setting is
  // 'pono::UNKNOWN', indicating that options were correctly set and
  // 'ERROR' otherwise, e.g. wrong command line options or
  // incompatible options were passed
  assert(res == pono::UNKNOWN);

  // set logger verbosity -- can only be set once
  logger.set_verbosity(pono_options.verbosity_);

  // For profiling: set signal handlers for common signals to abort
  // program.  This is necessary to gracefully stop profiling when,
  // e.g., an external time limit is enforced to stop the program.
  if (!pono_options.profiling_log_filename_.empty()) {
    signal(SIGINT, profiling_sig_handler);
    signal(SIGTERM, profiling_sig_handler);
    signal(SIGALRM, profiling_sig_handler);
#ifdef WITH_PROFILING
    logger.log(
        0, "Profiling log filename: {}", pono_options.profiling_log_filename_);
    ProfilerStart(pono_options.profiling_log_filename_.c_str());
#endif
  }

#ifdef NDEBUG
  try {
#endif
    // no logging by default
    // could create an option for logging solvers in the future

    // HACK bool_model_generation for IC3IA breaks CegProphecyArrays
    // longer term fix will use a different solver in CegProphecyArrays,
    // but for now just force full model generation in that case

    SmtSolver s = create_solver_for(pono_options.smt_solver_,
                                    pono_options.engine_,
                                    false,
                                    pono_options.ceg_prophecy_arrays_);

    if (pono_options.logging_smt_solver_) {
      s = make_shared<LoggingSolver>(s);
      // TODO consider setting base-context-1 for BTOR here
      //      to allow resetting assertions
    }

    // limitations with COI
    if (pono_options.static_coi_) {
      if (pono_options.pseudo_init_prop_) {
        // Issue explained here:
        // https://github.com/upscale-project/pono/pull/160 will be resolved
        // once state variables removed by COI are removed from init then should
        // do static-coi BEFORE mod-init-prop
        logger.log(0,
                   "Warning: --mod-init-prop and --static-coi don't work "
                   "well together currently.");
      }
    }
    // default options for IC3SA
    if (pono_options.engine_ == IC3SA_ENGINE) {
      // IC3SA expects all state variables
      pono_options.promote_inputvars_ = true;
    }

    // TODO: make this less ugly, just need to keep it in scope if using
    //       it would be better to have a generic encoder
    //       and also only create the transition system once
    string file_ext = pono_options.filename_.substr(
        pono_options.filename_.find_last_of(".") + 1);
    if (file_ext == "btor2" || file_ext == "btor") {
      logger.log(2, "Parsing BTOR2 file: {}", pono_options.filename_);
      FunctionalTransitionSystem fts(s);
      BTOR2Encoder btor_enc(pono_options.filename_, fts);
      const TermVec & propvec = btor_enc.propvec();
      unsigned int num_props = propvec.size();
      if (pono_options.prop_idx_ >= num_props) {
        throw PonoException(
            "Property index " + to_string(pono_options.prop_idx_)
            + " is greater than the number of properties in file "
            + pono_options.filename_ + " (" + to_string(num_props) + ")");
      }

      Term prop = propvec[pono_options.prop_idx_];

      vector<UnorderedTermMap> cex;
      res = check_prop(pono_options, prop, fts, s, cex);
      // we assume that a prover never returns 'ERROR'
      assert(res != ERROR);

      // print btor output
      if (res == FALSE) {
        cout << "sat" << endl;
        cout << "b" << pono_options.prop_idx_ << endl;
        assert(pono_options.witness_ || !cex.size());
        if (cex.size()) {
          print_witness_btor(btor_enc, cex, fts);
          if (!pono_options.vcd_name_.empty()) {
            VCDWitnessPrinter vcdprinter(fts, cex);
            vcdprinter.dump_trace_to_file(pono_options.vcd_name_);
          }
        }
      } else if (res == TRUE) {
        cout << "unsat" << endl;
        cout << "b" << pono_options.prop_idx_ << endl;
      } else {
        assert(res == pono::UNKNOWN);
        cout << "unknown" << endl;
        cout << "b" << pono_options.prop_idx_ << endl;
      }

    } else if (file_ext == "smv" || file_ext == "vmt" || file_ext == "smt2") {
      logger.log(2, "Parsing SMV/VMT file: {}", pono_options.filename_);
      if (pono_options.timed_automaton_ && file_ext != "vmt"){
        throw PonoException("Timed automata are only supported in the .vmt format");
      }

      std::unique_ptr<RelationalTransitionSystem> rts = nullptr;
      TermVec propvec;
      if (pono_options.timed_automaton_ && file_ext == "vmt"){
        // The timed automaton case
        rts = std::make_unique<TimedTransitionSystem>(s);
        TimedVMTEncoder vmt_enc(pono_options.filename_, *dynamic_cast<TimedTransitionSystem*>(rts.get()));
        propvec = vmt_enc.propvec();
      } else {
        // The standard (non-timed automaton) case
        rts = std::make_unique<RelationalTransitionSystem>(s);
        if (file_ext == "smv") {
          SMVEncoder smv_enc(pono_options.filename_, *rts);
          propvec = smv_enc.propvec();
        } else {
          assert(file_ext == "vmt" || file_ext == "smt2" );
          VMTEncoder vmt_enc(pono_options.filename_, *rts);
          propvec = vmt_enc.propvec();
        }
      }
      unsigned int num_props = propvec.size();
      if (pono_options.prop_idx_ >= num_props) {
        throw PonoException(
            "Property index " + to_string(pono_options.prop_idx_)
            + " is greater than the number of properties in file "
            + pono_options.filename_ + " (" + to_string(num_props) + ")");
      }

      Term prop = propvec[pono_options.prop_idx_];
      // get property name before it is rewritten

      std::vector<UnorderedTermMap> cex;
      res = check_prop(pono_options, prop, *rts, s, cex);
      // we assume that a prover never returns 'ERROR'
      assert(res != ERROR);

      logger.log(
          0, "Property {} is {}", pono_options.prop_idx_, to_string(res));

      if (res == FALSE) {
        cout << "sat" << endl;
        assert(pono_options.witness_ || cex.size() == 0);
        for (size_t t = 0; t < cex.size(); t++) {
          cout << "AT TIME " << t << endl;
          for (auto elem : cex[t]) {
            cout << "\t" << elem.first << " : " << elem.second << endl;
          }
        }
        assert(pono_options.witness_ || pono_options.vcd_name_.empty());
        if (!pono_options.vcd_name_.empty()) {
          VCDWitnessPrinter vcdprinter(*rts, cex);
          vcdprinter.dump_trace_to_file(pono_options.vcd_name_);
        }
      } else if (res == TRUE) {
        cout << "unsat" << endl;
      } else {
        assert(res == pono::UNKNOWN);
        cout << "unknown" << endl;
      }
    } 
    else {
      throw PonoException("Unrecognized file extension " + file_ext
                          + " for file " + pono_options.filename_);
    }
#ifdef NDEBUG
  }
  catch (PonoException & ce) {
    cout << ce.what() << endl;
    cout << "error" << endl;
    cout << "b" << pono_options.prop_idx_ << endl;
    res = ProverResult::ERROR;
  }
  catch (SmtException & se) {
    cout << se.what() << endl;
    cout << "error" << endl;
    cout << "b" << pono_options.prop_idx_ << endl;
    res = ProverResult::ERROR;
  }
  catch (std::exception & e) {
    cout << "Caught generic exception..." << endl;
    cout << e.what() << endl;
    cout << "error" << endl;
    cout << "b" << pono_options.prop_idx_ << endl;
    res = ProverResult::ERROR;
  }
#endif

  if (!pono_options.profiling_log_filename_.empty()) {
#ifdef WITH_PROFILING
    ProfilerFlush();
    ProfilerStop();
#endif
  }
  if (pono_options.print_wall_time_) {
    auto end_time_stamp = timestamp();
    auto elapsed_time = timestamp_diff(begin_time_stamp, end_time_stamp);
    std:cout << "Pono wall clock time (s): " <<
      time_duration_to_sec_string(elapsed_time) << std::endl;
  }

  return res;
}
