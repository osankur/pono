# Pono Real-Time
This fork contains an extension of Pono for real-time:
- An encoder for timed automata from .vmt files
- Algorithms to check rt-consistency which means checking whether `AX not prop` is reachable.
(The system is rt-consistent if no such state is reachable).

Currently, this is supported within the engines BMC, IND, IC3IA via a small variant supporting quantified properties and custom interpolators.

Note that recent versions of Mathsat are not currently compatible with Pono. Please use version 5.6.4

## Timed Automata
Timed automata are encoded in the [VMT](https://vmt-lib.fbk.eu/) format. In addition to
a regular VMT file, the input file specifies clocks (any variable for which the attribute `:nextclock` is used is a clock),
location invariants (with the `:locinvar` attribute), and urgent loctations (`:urgent`).

- `locinvar` defines the location invariant. In case of multiple occurrences, the conjunction of all is the invariant.
A locinvar expression can only use current variables. (TODO add a check)
- `urgent` defines the set of states where time cannot elapse. In case of multiple occurrences, the disjunction of all is the set of urgent states. An urgent expression can only use current nonclock variables. (TODO add a check)

The input file must encode the discrete transitions; delay transitions are added by Pono.
An example is `samples/rtc/simple_ta.vmt`.

One can run the k-induction algorithm as follows.

    pono -e ind --smt-solver cvc5 -ta --witness samples/rtc/simple_ta.vmt
    pono -e ind --smt-solver cvc5 -ta --witness samples/rtc/simple_ta_2.vmt

The `-ta` option is to specify that the file encodes a timed automaton.
One also needs an SMT solver supporting linear theory of reals e.g. cvc5.

## Using RT-Consistency Algorithms
RT-Consistency check is relevant both for Boolean systems and timed automata.

BMC or k-induction can be used out of the box using a solver supporting quantifiers (CVC5 or Z3).
Specifying the `--rt-consistency` option will rewrite the property `P` given in the input file as 

    Qprop = ~(P(X) /\ ∀ Y. ∀I. T(X, I, Y) -> ~P(Y))

There are two rt-consistency algorithms: the static (0) one eliminates quantifiers from the above formula,
and applies any standard algorithm; the dynamic (1) one keeps the quantified formula.

The BMC and k-induction algorithms can be run to check rt-consistency with the dynamic algorithm as follows.

    pono -e bmc --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
    pono -e ind --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
    pono -e ind --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_inconsistent.smv

Use --rt-consistency 0 for the static algorithm.

The tool answers `unsat` if there is no inconsistency (~Qprop is not reachable),
`sat` if it finds a counterexample, and `unknown` otherwise.
Note that the BMC answers Unknown above since it can only detect counterexamples.

On an rt-inconsistent input, a witness can be displayed with the `--witness` option:

    pono -e bmc --smt-solver cvc5 --rt-consistency 0 --witness samples/rtc/sample_inconsistent.smv

The ic3ia engine can be used to check rt-consistency statically provided that an smt-solver with quantifier elimination is provided.
In this case, the interpolator is msat:

    pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 samples/rtc/sample_consistent.smv
    pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 --witness samples/rtc/sample_inconsistent.smv

(Witness generation to ic3ia was added in this fork).
Note that the witness is a path to a state whose all successors violate the given property.

A variant of the ic3ia algorithm is implemented as well to check rt-consistency dynamically. Again the interpolator by default is msat.

    pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
    pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_inconsistent.smv

RT-consistency of timed automata, using both algorithms:
    pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 1 samples/rtc/simple_ta.vmt
    pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 0 samples/rtc/simple_ta.vmt

It is possible to specify a custom interpolator with the --interpolator option.

## Installation
The following packages are required for compilation to succeed

- flex, bison
- libgmp-dev
- Cython
- pip3 install toml scikit
- libbison-dev

## Testing
To compile the tests uncomment the following lines in CMakeLists.txt

    enable_testing()
    add_subdirectory(tests)

Then `./build/tests/test_ta` tests timed automata semantics.