/*********************                                                  */
/*! \file IC3IARTC.cpp
** \brief IC3IA applied to rt-consistency checking.
** \author Ocan Sankur <ocan.sankur@cnrs.fr>
**
**/

#include "engines/ic3iartc.h"

#include <random>

#include "smt/available_solvers.h"
#include "utils/logger.h"
#include "utils/term_analysis.h"

using namespace smt;
using namespace std;

namespace pono {

IC3IARTC::IC3IARTC(const Property & p,
                   const TransitionSystem & ts,
                   const SmtSolver & s,
                   PonoOptions opt)
    : IC3IA(p, ts, s, opt)
{
  assert(ts_.solver() == orig_property_.solver());
}

/**
 * This is a copy of IC3IA::reconstruct_trace with the following difference.
 * Rather than adding bad at the end of the trace, we actually extract a bad
 * state that is reachable from the penultimate step. This helps us avoid having
 * a quantified formula in the trace.
 */
void IC3IARTC::reconstruct_trace(const ProofGoal * pg, TermVec & out)
{
  assert(!solver_context_);
  assert(pg);
  assert(pg->target.term);
  assert(check_intersects_initial(pg->target.term));

  logger.log(2, "IC3IARTC::reconstruct_trace\n");
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
  // std::cout << "Solving for:\n";
  // std::cout << "\t" << out.back() << "\n";
  // std::cout << "\t" << ts_.next(bad_) << "\n";
  // std::cout << "\t" << ts_.trans() << "\n";

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
 * Extract a model for next-state variables from the last call to the solver,
 * and return a current-state version of that assignment.
 *
 * TODO: do we want an assignment on concrete states or predicates?
 */
Term IC3IARTC::get_nextstate_model() const
{
  TermVec conjuncts;
  conjuncts.reserve(predlbls_.size());
  Term val;
  for (const auto & p : predlbls_) {
    if ((val = solver_->get_value(ts_.next(p))) == solver_true_) {
      conjuncts.push_back(lbl2pred_.at(p));
    } else {
      conjuncts.push_back(solver_->make_term(Not, lbl2pred_.at(p)));
    }
    assert(val->is_value());
  }

  return make_and(conjuncts, this->solver_);
}

/**
 * This is a copy of IC3IA::initialize with the following difference:
 * we do not look into bad_ to extract predicates (because currently,
 * get_predicates fail at quantifiers) We could also comment out
 * get_free_symbols for bad_ for the same reason.
 */
void IC3IARTC::initialize()
{
  if (initialized_) {
    return;
  }

  // Skip initialize of IC3IA. The present function is a copy with a simple
  // modification
  super::super::initialize();

  bad_ = getQuantifiedRTConsistencyBad();
  cout << "RTC Bad property: " << bad_->to_string() <<'\n';

  // add all the predicates from init and property to the abstraction
  // NOTE: abstract is called automatically in IC3Base initialize
  UnorderedTermSet preds;
  get_predicates(solver_, conc_ts_.init(), preds, false, false, true);
  size_t num_init_preds = preds.size();
  size_t num_prop_preds = 0;
  // get_predicates(solver_, bad_, preds, false, false, true);
  for (const auto & p : preds) {
    add_predicate(p);
  }
  logger.log(1, "Number predicates found in init: {}", num_init_preds);
  logger.log(1, "Number predicates found in prop: {}", num_prop_preds);
  logger.log(1, "Total number of initial predicates: {}", preds.size());
  // more predicates will be added during refinement
  // these ones are just initial predicates

  // populate cache for existing terms in solver_
  UnorderedTermMap & cache = to_solver_.get_cache();
  Term ns;
  for (auto const & s : ts_.statevars()) {
    // common variables are next states, unless used for refinement in IC3IA
    // then will refer to current state variables after untiming
    // need to cache both
    cache[to_interpolator_.transfer_term(s)] = s;
    ns = ts_.next(s);
    cache[to_interpolator_.transfer_term(ns)] = ns;
  }

  // need to add uninterpreted functions as well
  // first need to find them all
  // NOTE need to use get_free_symbols NOT get_free_symbolic_consts
  // because the latter ignores uninterpreted functions
  UnorderedTermSet free_symbols;
  get_free_symbols(ts_.init(), free_symbols);
  get_free_symbols(ts_.trans(), free_symbols);
  get_free_symbols(bad_, free_symbols);  // RTC

  for (auto const & s : free_symbols) {
    assert(s->is_symbol());
    if (s->is_symbolic_const()) {
      // ignore constants
      continue;
    }
    cache[to_interpolator_.transfer_term(s)] = s;
  }

  // TODO fix generalize_predecessor for ic3ia
  //      might need to override it
  //      behaves a bit differently with both concrete and abstract next state
  //      vars
  if (options_.ic3_pregen_) {
    logger.log(1,
               "WARNING automatically disabling predecessor generalization -- "
               "not supported in IC3IA yet.");
    options_.ic3_pregen_ = false;
  }
}

/**
 * Return the following quantified formula:
 * P(X) /\ ∀ Y. ∀I. T(X, I, Y) -> ~P(Y)
 */
Term IC3IARTC::getQuantifiedRTConsistencyBad() {
  // Map nextstatevar to param
  UnorderedTermMap subst;
  // for (const auto & sv : ts_.statevars()) {
  //   cout << "State var: " << sv->to_string() << "\n";
  // }
  // for (const auto & sv : ts_.inputvars()) {
  //   cout << "Input var: " << sv->to_string() << "\n";
  // }

  for (const auto & sv : ts_.statevars()) {
    const Sort & sort = sv->get_sort();
    Term p = solver_->make_param("#" + sv->to_string(), sort);
    this->var2param_[sv] = p;
    subst[ts_.next(sv)] = p;
    this->param2var_[p] = sv;
  }
  for (const auto & sv : ts_.inputvars()) {
    const Sort & sort = sv->get_sort();
    Term p = solver_->make_param("#" + sv->to_string(), sort);
    this->var2param_[sv] = p;
    subst[sv] = p;
    this->param2var_[p] = sv;
  }

  // std::cout << "trans: " << ts_.trans() << "\n";
  Term transXY = solver_->substitute(ts_.trans(), subst);
  // std::cout << "transXY: " << transXY << "\n";
  Term badY = solver_->substitute(bad_, var2param_);
  Term rhs = solver_->make_term(Implies, transXY, badY);
  for (auto pv : param2var_) {
    rhs = solver_->make_term(Forall, pv.first, rhs);
  }
  return solver_->make_term(And, solver_->make_term(Not, bad_), rhs);
}

}  // namespace pono