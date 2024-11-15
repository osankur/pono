# Pono Real-Time
This fork of Pono contains a `real-time' extension of Pono:
- A .vmt front-end for timed automata
- Algorithms to check real-time-consistency (rt-consistency) which means checking whether a state satisfying `prop /\ (AX not prop)` is reachable.
(The system is rt-consistent if no such state is reachable).

Currently, this is supported either by eliminating quantifiers and then using any engine, or keeping the property quantified within engines BMC, IND, and IC3IA via a small variant supporting quantified properties and custom interpolators.

Note that recent versions of Mathsat are not currently compatible with Pono. Please use version 5.6.4

The installation script of this fork uses a fork of smt-switch with several bug fixes and support for quantifier elimination.

## Timed Automata
Timed automata are encoded in the [VMT](https://vmt-lib.fbk.eu/) format as used in the nuXmv tool. In addition to
a regular VMT file, the input file specifies clocks (any variable for which the attribute `:nextclock` is used is a clock),
location invariants (with the `:locinvar` attribute), and urgent loctations (`:urgent`).

- `locinvar` defines the location invariant. In case of multiple occurrences, the conjunction of all is the invariant.
A locinvar expression can only use current variables. (TODO add a check)
- `urgent` defines the set of states where time cannot elapse. In case of multiple occurrences, the disjunction of all is the set of urgent states. An urgent expression can only use current nonclock variables. (TODO add a check)

The input file must encode only the discrete transitions; delay transitions are added by Pono.
An example is `samples/rtc/simple_ta.vmt`.

Clocks variables can be declared either as Real or Int, but all clocks must be of the same type.
The semantics considered by Pono is the following: each atomic step is a combination of a discrete transition, followed by an arbitrary delay.
(A dummy edge is added at initial locations to allow starting the run with a delay).

The timed automaton semantics is built from the given input `.vmt` file when option `-ta` is given.
One needs to use an SMT solver supporting linear theory of reals e.g. cvc5.
For instance,

    pono -e ind --smt-solver cvc5 -ta --witness samples/rtc/simple_ta.vmt
    pono -e ind --smt-solver cvc5 -ta --witness samples/rtc/simple_ta_2.vmt
    pono -e ind --smt-solver cvc5 -ta -p 1 --witness samples/rtc/simple_ta_2.vmt

Here the option `-p 1` is used to specify that property of index 1 is to be checked (the default is 0).
The properties are numbered from 0 in the order of their appearances in the .vmt file.

The file `simple_ta_2_int.vmt` is identical to `simple_ta_2.vmt` with the exception that the clocks are integers:

    pono -e ind --smt-solver cvc5 -ta --witness samples/rtc/simple_ta_2_int.vmt

Here, Pono produces an integer-valued counterexample trace, as expected.

The second property holds with integer delays (because any counterexample requires a delay of < 1):

    pono -e ind --smt-solver cvc5 -ta --witness -p 1 samples/rtc/simple_ta_2_int.vmt

There is also support for a semantics with unit delays, where each atomic step is a discrete transition followed by a delay of exactly 1.
This is activated with option `-ta-unit`. For instance,

    pono -e ind --smt-solver cvc5 -ta-unit --witness samples/rtc/simple_ta_2_int.vmt

Here, no counterexample is found since it is not possible to violate the property by taking discrete transitions with unit delays.
Note that unit-delay semantics is slower to analyze in general because it generates a great number of disctinct intermediate states.

## Checking RT-Consistency
RT-Consistency check is relevant both for Boolean systems and timed automata.
Three engines can be used to check rt-consistency: bmc, ind, and ic3ia.

BMC or k-induction can be used out of the box using a solver supporting quantifiers (such as CVC5).
Specifying the `--rt-consistency` option will rewrite the property `P` given in the input file as 

    Qprop = ~(P(X) /\ ∀ Y. ∀I. T(X, I, Y) -> ~P(Y))

There are two rt-consistency algorithms: the static (0) one eliminates quantifiers from the above formula,
and applies any standard algorithm; the dynamic (1) one keeps the quantified formula.

The BMC and k-induction algorithms can be run to check rt-consistency with the dynamic algorithm as follows.

    pono -e bmc --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
    pono -e ind --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
    pono -e ind --smt-solver cvc5 --rt-consistency 1 --witness samples/rtc/sample_inconsistent.smv

Use --rt-consistency 0 for the static algorithm.
Note that the k-induction engine does not scale to larger models (because it creates k copies of Qprop to check k-inductiveness, which becomes hard to solve).

The tool answers `unsat` if there is no inconsistency (~Qprop is not reachable),
`sat` if it finds a counterexample, and `unknown` otherwise.
Note that the BMC answers Unknown above since it can only detect counterexamples.

The ic3ia engine can be used to check rt-consistency statically provided that an smt-solver with quantifier elimination is provided.
The default interpolator is msat (see below to use open-source interpolators)

    pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 samples/rtc/sample_consistent.smv
    pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 --witness samples/rtc/sample_inconsistent.smv

(Witness generation to ic3ia was added in this fork).
Here, the witness is a path to a state whose all successors violate the given property.

A variant of the ic3ia algorithm is implemented as well to check rt-consistency dynamically. Again the interpolator by default is msat.
The ic3ia algorithm and its refinement were modified to work with quantified properties:

    pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
    pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 --witness samples/rtc/sample_inconsistent.smv

RT-consistency of timed automata, using both algorithms:
    pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 1 samples/rtc/simple_ta.vmt
    pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 0 samples/rtc/simple_ta.vmt

## Deadlocks as RT-Inconsistencies
A side-effect of the rt-consistency check is that any deadlock (that is, a reachable state with no outgoing transition) is considered an rt-inconsistency since such a state (valuation over X) satisfies `∀ Y. ∀I. T(X, I, Y) -> ~P(Y)`. So one must be careful to avoid deadlocks in the model or make sure they are intended.

## IC3IA with external interpolators
If MathSAT is not available, there is support for the following interpolators which are free software: opensmt, smtinterpoal.
These can be installed by using the scripts `contrib/setup_opensmt.sh` and `contrib/setup_smtinterpol.sh` which will download the executables, and the folders containing them to the path (to `~/.bashrc`).

    pono -e ic3ia --smt-solver cvc5 -ta --external-interpolator opensmt samples/rtc/simple_ta.vmt
    pono -e ic3ia --smt-solver cvc5 -ta --external-interpolator smtinterpol samples/rtc/simple_ta.vmt

The support is rather fragile due to the incomplete smtlib parser of Pono and inconsistencies between the use of this format among different solvers. For instance, Pono cannot parse real numbers (such as 1.0) in the smt files; these must be given as (to_real 1). There is a workaround but it will fail for a number such as 1.2.

## Installation
Installation instructions follow those in the main [README-pono](README-pono) with a few more details.

The following packages are required for the compilation to succeed on Ubuntu 22.04:
- flex, bison
- libbison-dev
- libgmp-dev
- build-essential
- Java >= 1.8 (e.g. the Ubuntu package openjdk-18-jdk)

And for Python bindings:
- cythons3 python3-dev
- pip3 install toml scikit-learn

Please use g++-11; compilation fails with clang due to some warnings being counted as errors.

Install first custom smt solvers and the smt-switch interface. 

    cd contrib
    ./setup-smtinterpol.sh
    ./setup-opensmt.sh
    ./setup-btor2tools.sh

If you have MathSAT version 5.6.4, extract it to `deps/mathsat`, and run

    ./setup-smt-switch.sh --with-msat

To compile without MathSAT, just run

    ./setup-smt-switch.sh
    
Then

    cd ..
    ./configure.sh

If building with mathsat, also include `--with-msat` as an option to `configure.sh`.

    cd build
    make 

## Testing
To compile the tests uncomment the following lines in CMakeLists.txt

    enable_testing()
    add_subdirectory(tests)

Then `./build/tests/test_ta` tests timed automata semantics.

The following cram test file contains all examples in this readme:

    tests/cram/ta.t
