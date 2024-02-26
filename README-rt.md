# Pono Real-Time
This fork contains an extension of Pono for real-time:
- A parser for timed automata from .vmt files
- Algorithms to check rt-consistency which consists in checking whether `AX not prop` is reachable.
(The system is rt-consistent if no such state is reachable).
Currently, this is supported within the engine KIND, and the new engine IC3IARTC.

## Using RT-Consistency Algorithms
- BMC can be used out of the box using a solver supporting quantifiers (CVC5 or Z3).
Specifying the `--rt-consistency` option will rewrite the property `P` given in the input file as 

    Qprop = ~(P(X) /\ ∀ Y. ∀I. T(X, I, Y) -> ~P(Y))

The BMC and k-induction algorithms to check rt-consistency can be run as follows.

    ./pono -e bmc --smt-solver cvc5 --rt-consistency ../samples/rtc/sample_consistent.smv
    ./pono -e ind --smt-solver cvc5 --rt-consistency ../samples/rtc/sample_consistent.smv

The tool answers `unsat` if there is no inconsistency (~Qprop is not reachable),
sat if it finds a counterexample, and `unknown` otherwise.
Note that the BMC answers Unknown above since it can only detect counterexamples.

On an rt-inconsistent input, a witness can be displayed with the `--witness` option:

    ./pono -e bmc --smt-solver cvc5 --rt-consistency --witness ../samples/rtc/sample_consistent.smv

## Installation
The following packages are required for compilation to succeed

- flex, bison
- libgmp-devA
- Cython
- pip3 install toml scikit
- libbison-dev