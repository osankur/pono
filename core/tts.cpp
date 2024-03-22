#include "core/tts.h"

#include <cassert>
#include "utils/logger.h"
#include "smt-switch/term.h"
using namespace smt;
namespace pono {
const std::string TimedTransitionSystem::DELAY_VAR_NAME = "_DELTA_";

void TimedTransitionSystem::encode_timed_automaton_delays(
    const TimedAutomatonEncoding & encoding)
{
  if (encoded_delays_) return;
  this->encoded_delays_ = true;
  switch (encoding) {
    case TimedAutomatonEncoding::Compact: 
      encode_compact_delays();
      break;
    default:
      throw std::runtime_error("Non-compact delays are not yet implemented");
  }
}

void TimedTransitionSystem::add_dummy_init_transitions(){
  assert(!this->has_dummy_init_transitions_);
  this->has_dummy_init_transitions_ = true;

  smt::Term dummy_transition = solver_->make_term(true);  
  for (auto v : statevars_) {
    smt::Term vunchanged = solver_->make_term(Equal, v, next(v));
    dummy_transition = solver_->make_term(And, dummy_transition, vunchanged);
  }
  dummy_transition = solver_->make_term(And, dummy_transition, init());
  logger.log(4, "Dummy transitions: {}", dummy_transition);
  // dummy_transition = X'=X/\ C'=C /\ init
  // trans := trans \/ dummy_transition
  set_trans(solver_->make_term(Or, trans(), dummy_transition));
}

void TimedTransitionSystem::encode_compact_delays(){
  add_dummy_init_transitions();
  smt::Sort realsort = solver_->make_sort(REAL);
  delta_ = TransitionSystem::make_inputvar(DELAY_VAR_NAME, realsort);
  /*
     * T(C,X, I, C',X') =
     *    C >= 0 
     * /\ delta >= 0 
     * /\ locinvar(C,X)
     * /\ (urgent(X') -> delta = 0) 
     * /\ (trans(C,X,I,C'-delta,X')
     * /\ locinvar(X',C')
     */
  smt::Term zero = solver_->make_term("0", realsort);
  smt::Term clocks_nonnegative = solver_->make_term(true);
  smt::Term clocks_are_zero = solver_->make_term(true);
  for (auto c : clock_vars_) {
    clocks_are_zero = 
      solver_->make_term(And, 
        clocks_are_zero, 
        solver_->make_term(Equal, c, zero));
  }
  logger.log(4, "TA Clock >= 0: {}", clocks_nonnegative);
  smt::Term new_trans = clocks_nonnegative;

  smt::Term delta_nonnegative = solver_->make_term(Le, zero, delta_);
  logger.log(4, "TA delta >= 0: {}", delta_nonnegative);
  new_trans = solver_->make_term(And, new_trans, delta_nonnegative);

  smt::Term delta0ifurgent = 
    solver_->make_term(Implies, next(urgent()), solver_->make_term(Equal, delta_, zero));
  new_trans = solver_->make_term(And, new_trans, delta0ifurgent);
  logger.log(4, "TA delta0ifurgent: {}", delta0ifurgent);

  smt::UnorderedTermMap Cp2Cpmdelta;
  for (auto c : clock_vars_) {
    Cp2Cpmdelta[next(c)] = solver_->make_term(Minus, next(c), delta_);
  }
  logger.log(4, "TA Cp2Cpmdelta: {}", solver_->substitute(trans(), Cp2Cpmdelta));
  new_trans = solver_->make_term(And, new_trans, solver_->substitute(trans(), Cp2Cpmdelta));

  new_trans = solver_->make_term(And, new_trans, locinvar());
  new_trans = solver_->make_term(And, new_trans, next(locinvar()));

  // trans = discrete \/ time_elapse
  set_trans(new_trans);
  set_init(solver_->make_term(And, init(), clocks_are_zero));
  // init = init /\ C = 0
  logger.log(4, "TA Init: {}", init());
  logger.log(4, "TA locinvar: {}", locinvar());
  logger.log(4, "TA urgent: {}", urgent());
}
}  // namespace pono