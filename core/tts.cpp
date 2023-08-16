#include "smt-switch/term.h"
#include "core/tts.h"
using namespace smt;
namespace pono{
    const std::string TimedTransitionSystem::GLOBAL_CLOCK_VAR_NAME = "#global_time";

    void TimedTransitionSystem::initialize(){
        // Register nonclock and clock variables
        for (const auto & sv : statevars()) {
            switch(sv->get_sort()->get_sort_kind()){
                case smt::SortKind::REAL:
                clock_vars_.insert(sv);
                break;
                default:
                nonclock_vars_.insert(sv);
                // throw PonoException("The timed automaton solver only accepts bool and real sorts. Got " + sv->get_sort()->to_string());
            }
        }

        smt::Sort realsort = solver_->make_sort(REAL);
        Term delta = TransitionSystem::make_statevar(GLOBAL_CLOCK_VAR_NAME, realsort);
        // delta = 0
        Term delta_init = solver_->make_term(Equal, delta, solver_->make_term((int64_t) 0, realsort));
        
        set_init(solver_->make_term(And, original_ts_.init(), delta_init));

        // time_elapse = delta <= delta' /\\ v' = v (for all nonclock v) /\\ x'=x+delta'-delta
        Term time_elapse = solver_->make_term(Le, delta, next(delta));
        for(auto & v : nonclock_vars_){
            time_elapse = solver_->make_term(And, time_elapse, 
                solver_->make_term(Equal, next(v), v));
        }
        for(auto & x : clock_vars_){
            time_elapse = solver_->make_term(And, time_elapse, 
                solver_->make_term(Equal, next(x), 
                  solver_->make_term(Plus, x,
                    solver_->make_term(Minus, next(delta), delta)
                  )
                ));
        }
        time_elapse = solver_->make_term(And, time_elapse, invariant());
        time_elapse = solver_->make_term(And, time_elapse, next(invariant()));

        // discrete = delta' = delta /\ trans
        Term discrete = solver_->make_term(Equal, delta, next(delta));
        discrete = solver_->make_term(And, discrete, trans());

        set_trans(solver_->make_term(Or, discrete, time_elapse));
        std::cout << "TA Init: " << init() << "\n";
        std::cout << "TA Discrete: " << discrete << "\n";
        std::cout << "TA Time elapse: " << time_elapse << "\n";

        for(auto & x : clock_vars_){
          std::cout << "Clock " << x->to_string() << "\n";
        }

    }
} 