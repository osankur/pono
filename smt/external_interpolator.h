#pragma once


#include <iostream>

#include "options/options.h"
#include "frontends/vmt_encoder.h"
#include "smt-switch/smt.h"
#include "smt-switch/smtlib_reader.h"
#include "smt/available_solvers.h"
#include "smt-switch/term_translator.h"
#include "smt-switch/smt.h"

namespace pono{

/**
 * A light-weight interface to OpenSMT by calling it as an external program.
*/
class ExternalInterpolator {
  public:
  /**
   * @arg original_interpolator a solver for which the produced interpolants will be transferred.
  */
  ExternalInterpolator(smt::SmtSolver & original_interpolator, ExternalInterpolatorEnum externalInterpolator = OPENSMT) 
    : original_interpolator_(original_interpolator),
      solver_(create_solver(smt::CVC5)),
      to_original_interpolator_(original_interpolator),
      externalInterpolator_(externalInterpolator)
      {
  }
  smt::Result get_interpolant(const smt::Term & A, const smt::Term & B, smt::Term & out_I);
  smt::Result get_sequence_interpolants(const smt::TermVec & formulae, smt::TermVec & outInterpolants);

  std::string executable(){
    switch(externalInterpolator_){
      case OPENSMT: return "opensmt";
      case Z3: return "z3-4.7.1";
      case SMTINTERPOL:
        return "smtinterpol";
      default: std::runtime_error("unknown external interpolator");
      return "";
    }
  }

  private:
    class InterpolantReader : public smt::SmtLibReader {
      public:
      InterpolantReader(std::string filename, smt::SmtSolver solver) : smt::SmtLibReader(solver), interpolant_(nullptr){
        set_logic_all();
        int res = parse(filename);
        assert(!res);  // 0 means success
      }
      void term_attribute(const smt::Term & term,
                                     const std::string & keyword,
                                     const std::string & value)
      {
        if (keyword == "interpolant") {
          this->interpolant_ = term;
        }   
      }
      void new_symbol(const std::string & name, const smt::Sort & sort){}
      smt::Term get_interpolant(){
        assert(interpolant_);
        return interpolant_;
      }
      private:
      smt::Term interpolant_;
    };

    // Prepare the smt query by replacing (to_real N) -> N
    std::string remove_to_real(std::string query);
    std::string insert_to_real(std::string query);
    // Replace -N -> (- N)
    std::string clean_negative_numbers(std::string query);
    /** 
     * @brief Call external solver to execute query, parse output and write resulting interpolants in outInterpolants
     * @arg symbols set of free symbols appearing in query
     * @pre symbols contains the set of free symbols appearing in query
     */
    smt::Result execute_query(std::string & query, const smt::UnorderedTermSet & symbols, smt::TermVec & outInterpolants);
    smt::SmtSolver & original_interpolator_;
    smt::SmtSolver solver_;
    // smt::TermTranslator to_solver_;
    smt::TermTranslator to_original_interpolator_;
    ExternalInterpolatorEnum externalInterpolator_;
};

}