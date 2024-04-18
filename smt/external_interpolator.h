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
   * @arg original_interpolator a solver for which the produced interpolants will be transferred:
   * \a get_interpolant and \a get_sequence_interpolants return terms for \a original_interpolator_
   * @arg externalInterpolator 
  */
  ExternalInterpolator(smt::SmtSolver & original_interpolator, ExternalInterpolatorEnum externalInterpolator = ExternalInterpolatorEnum::OPENSMT) 
    : original_interpolator_(original_interpolator),
      to_original_interpolator_(original_interpolator),
      externalInterpolator_(externalInterpolator)
      {
      }
  smt::Result get_interpolant(const smt::Term & A, const smt::Term & B, smt::Term & out_I);
  smt::Result get_sequence_interpolants(const smt::TermVec & formulae, smt::TermVec & outInterpolants);
  ExternalInterpolatorEnum getSolverEnum() const {
    return externalInterpolator_;
  }
  std::string executable(){
    switch(externalInterpolator_){
      case ExternalInterpolatorEnum::OPENSMT: return "opensmt";
      case ExternalInterpolatorEnum::Z3: return "z3-4.7.1";
      case ExternalInterpolatorEnum::SMTINTERPOL: return "smtinterpol";
      default: std::runtime_error("unknown external interpolator");
      return "";
    }
  }

  private:
    class InterpolantReader : public smt::SmtLibReader {
      public:
      /**
       * @pre all variable declarations in the given file \a filename must have been declared in solver.
      */
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
      // void new_symbol(const std::string & name, const smt::Sort & sort){
        // disabled because all variables must already be declared in the given solver
      // }
      smt::Term get_interpolant(){
        return interpolant_;
      }
      private:
      smt::Term interpolant_;
    };

    // Prepare the smt query by replacing (to_real N) -> N
    std::string remove_to_real(std::string query);
    std::string insert_to_real(std::string query);
    // Remove all occurrences of 'false'
    std::string remove_false(std::string query);
    // Replace -N -> (- N)
    std::string clean_negative_numbers(std::string query);
    /** 
     * @brief Call external solver to execute query, parse output and write resulting interpolants in outInterpolants
     * @arg symbols set of free symbols appearing in query
     * @pre symbols contains the set of free symbols appearing in query
     */
    smt::Result execute_query(std::string & query, const smt::UnorderedTermSet & symbols, smt::TermVec & outInterpolants);
    // Solver towards which computed interpolants will be transferred 
    smt::SmtSolver & original_interpolator_;
    smt::TermTranslator to_original_interpolator_;
    ExternalInterpolatorEnum externalInterpolator_;
};

}