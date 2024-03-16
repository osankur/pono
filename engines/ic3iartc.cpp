/*********************                                                  */
/*! \file IC3IAQ.cpp
** \brief IC3IA applied to rt-consistency checking.
** \author Ocan Sankur <ocan.sankur@cnrs.fr>
**
**/

#include "engines/ic3iartc.h"

#include <random>

#include "smt/available_solvers.h"
#include "smt/external_interpolator.h"
#include "utils/logger.h"
#include "utils/term_analysis.h"

using namespace smt;
using namespace std;

namespace pono {

IC3IAQ::IC3IAQ(const Property & p,
                   const TransitionSystem & ts,
                   const SmtSolver & s,
                   PonoOptions opt
                   )
    : IC3IA(p, ts, s, opt), external_interpolator_(interpolator_), use_external_interpolator_(opt.use_external_opensmt_interpolator_)
{
  logger.log(1, "Engine: IC3IAQ");
  engine_ = Engine::IC3IAQ_ENGINE;
  assert(ts_.solver() == orig_property_.solver());
}

/**
 * This is a copy of IC3IA::reconstruct_trace with the following difference.
 * Rather than adding bad at the end of the trace, we actually extract a bad
 * state that is reachable from the penultimate step. This helps us avoid having
 * a quantified formula in the trace.
 */
void IC3IAQ::reconstruct_trace(const ProofGoal * pg, TermVec & out)
{
  assert(!solver_context_);
  assert(pg);
  assert(pg->target.term);
  assert(check_intersects_initial(pg->target.term));

  logger.log(2, "IC3IAQ::reconstruct_trace\n");
  out.clear();
  while (pg) {
    out.push_back(pg->target.term);
    assert(ts_.only_curr(out.back()));
    pg = pg->next;
  }

  push_solver_context();
  solver_->assert_formula(out.back());
  solver_->assert_formula(ts_.next(bad_));
  solver_->assert_formula(ts_.trans());

  Result r = check_sat();
  assert(r.is_sat());

  Term prebad = get_nextstate_model();
  out.push_back(prebad);
  // std::cout << "Model: " << prebad->to_string() << "\n";
  pop_solver_context();

  // push_solver_context();
  // solver_->assert_formula(prebad);
  // solver_->assert_formula(bad_);
  // std::cout << "\nprebad is indeed in bad_ = " << check_sat() << "\n";
  // std::cout << "Solved for:\n";
  // std::cout << "\t" << prebad << "\n";
  // std::cout << "\t" << bad_ << "\n";
  // pop_solver_context();

  std::cout << "\nTRACE:\n";
  for (auto t : out ){
    std::cout << t->to_string() << "\n";
  }
}

/**
 * This is a copy of the IC3IA::refine function which can call an alternative interpolator.
 * We keep this as a copy here in order to minimize modifications to the core of Pono,
 * and because we have, for now, light-weight interfaces to alternative interpolators.
*/
RefineResult IC3IAQ::refine()
{
  logger.log(1, "\nIC3IAQ::Refinemenent with {} steps", cex_.size());
  // counterexample trace should have been populated
  assert(cex_.size());
  if (cex_.size() == 1) {
    std::cout << cex_.front()->to_string() << "\n";
    // if there are no transitions, then this is a concrete CEX
    return REFINE_NONE;
  }

  size_t cex_length = cex_.size();

  // use interpolator to get predicates
  // remember -- need to transfer between solvers
  assert(interpolator_ || use_external_interpolator_);

  TermVec formulae;
  for (size_t i = 0; i < cex_length; ++i) {
    // make sure to_solver_ cache is populated with unrolled symbols
    register_symbol_mappings(i);

    Term t = unroller_.at_time(cex_[i], i);
    if (i + 1 < cex_length) {
      t = solver_->make_term(And, t, unroller_.at_time(conc_ts_.trans(), i));
    }
    // std::cout << "Transferring " << t->to_string() << "\n";
    formulae.push_back(to_interpolator_.transfer_term(t, BOOL));
  }
  logger.log(1, "Getting seq interpolant for formulae:");
  for (auto f : formulae) {
    std::cout << f << "\n";
  }
  // interpolator_->reset_assertions();
  // std::cout << interpolator_->get_solver_enum() << "\n";
  // std::cout << "Asking for sequence interpolant:\n";
  // interpolator_->dump_smt2("/tmp/a.smt");

  // for (auto f : formulae){
  //   std::cout << f << "\n";
  // }
  TermVec out_interpolants;
  Result r = smt::ResultType::UNKNOWN;
  std::cout << "use opensmt: " << use_external_interpolator_ << "\n";
  if (use_external_interpolator_){
    r = external_interpolator_.get_sequence_interpolants(formulae, out_interpolants);
  } else {
    r =
      interpolator_->get_sequence_interpolants(formulae, out_interpolants);
  }

  if (r.is_sat()) {
    // this is a real counterexample, so the property is false
    logger.log(3, "Confirmed trace\n");
    return RefineResult::REFINE_NONE;
  }

  // record the length of this counterexample
  // important to set it here because it's used in register_symbol_mapping
  // to determine if state variables unrolled to a certain length
  // have already been cached in to_solver_
  longest_cex_length_ = cex_length;

  logger.log(3, "Seq interpolant size: ");
  // std::cout << "Interp size : " <<  + out_interpolants.size() << "\n";
  UnorderedTermSet preds;
  for (auto const&I : out_interpolants) {
    if (!I) {
      std::cout << "null\n";
      assert(
          r.is_unknown());  // should only have null terms if got unknown result
      continue;
    }

    Term solver_I = unroller_.untime(to_solver_.transfer_term(I, BOOL));
    assert(conc_ts_.only_curr(solver_I));
    logger.log(3, "got interpolant: {}", solver_I);
    get_predicates(solver_, solver_I, preds, false, false, true);
  }

  // new predicates
  TermVec fresh_preds;
  for (auto const&p : preds) {
    if (predset_.find(p) == predset_.end()) {
      // unseen predicate
      fresh_preds.push_back(p);
    }
  }

  if (!fresh_preds.size()) {
    logger.log(1, "IC3IA: refinement failed couldn't find any new predicates");
    return RefineResult::REFINE_FAIL;
  }

  if (options_.random_seed_ > 0) {
    shuffle(fresh_preds.begin(),
            fresh_preds.end(),
            default_random_engine(options_.random_seed_));
  }

  // reduce new predicates
  TermVec red_preds;
  if (options_.ic3ia_reduce_preds_
      && ia_.reduce_predicates(cex_, fresh_preds, red_preds)) {
    // reduction successful
    logger.log(2,
               "reduce predicates successful {}/{}",
               red_preds.size(),
               fresh_preds.size());
    if (red_preds.size() < fresh_preds.size()) {
      fresh_preds.clear();
      fresh_preds.insert(fresh_preds.end(), red_preds.begin(), red_preds.end());
    }
  } else {
    // if enabled should only fail if removed all predicates
    // this can happen when there are uninterpreted functions
    // the unrolling can force incompatible UF interpretations
    // but IC3 (which doesn't unroll) still needs the predicates
    // in this case, just use all the fresh predicates
    assert(!options_.ic3ia_reduce_preds_ || red_preds.size() == 0);
    logger.log(2, "reduce predicates FAILED");
  }

  // add all the new predicates
  for (auto const & p : fresh_preds) {
    bool new_pred = add_predicate(p);
    // expect all predicates to be new (e.g. unseen)
    // they were already filtered above
    assert(new_pred);
  }

  logger.log(1, "{} new predicates added by refinement", fresh_preds.size());

  // able to refine the system to rule out this abstract counterexample
  return RefineResult::REFINE_SUCCESS;
}


}  // namespace pono