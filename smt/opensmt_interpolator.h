#pragma once


#include <iostream>

#include "options/options.h"
#include "smt-switch/smt.h"
#include "smt/available_solvers.h"
#include "smt-switch/term_translator.h"

namespace pono{
/**
 * A light-weight interface to OpenSMT by calling it as an external program.
*/
class OpenSMTInterpolator {
  public:
  /**
   * @arg original_interpolator a solver for which the produced interpolants will be transferred.
  */
  OpenSMTInterpolator(smt::SmtSolver & original_interpolator) 
    : original_interpolator_(original_interpolator),
      solver_(create_solver(smt::MSAT)),
      to_solver_(solver_),
      to_original_interpolator_(original_interpolator){
    // todo check if opensmt is on the path
  }
  smt::Result get_interpolant(const smt::Term & A, const smt::Term & B, smt::Term & out_I);
  smt::Result get_sequence_interpolants(const smt::TermVec & formulae, smt::TermVec & outInterpolants);

  private:
    std::string clean_up_smt_query(std::string query) const;
    void execute_query(std::string query, smt::TermVec & outInterpolants) const;
    smt::SmtSolver & original_interpolator_;
    smt::SmtSolver solver_;
    smt::TermTranslator to_solver_;
    smt::TermTranslator to_original_interpolator_;
};

}