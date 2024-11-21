Testing simple reachability queries  
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 -ta -p 0 --witness samples/rtc/simple_ta_2.vmt
  Property 0 is FALSE
  sat
  AT TIME 0
  \t_DELTA_ : (/ 1 4) (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : 0.0 (esc)
  \tloc__AT0 : true (esc)
  \terr1 : false (esc)
  \terr2 : false (esc)
  AT TIME 1
  \t_DELTA_ : (/ 9 4) (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : (/ 1 4) (esc)
  \tloc__AT0 : false (esc)
  \terr1 : false (esc)
  \terr2 : false (esc)
  AT TIME 2
  \t_DELTA_ : 0.0 (esc)
  \tx__AT0 : (/ 9 4) (esc)
  \ty__AT0 : (/ 5 2) (esc)
  \tloc__AT0 : false (esc)
  \terr1 : true (esc)
  \terr2 : false (esc)
Cex generation with strict delays (default): 
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 -ta -p 1 --witness samples/rtc/simple_ta_2.vmt
  Property 1 is FALSE
  sat
  AT TIME 0
  \t_DELTA_ : (/ 1 6) (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : 0.0 (esc)
  \tloc__AT0 : true (esc)
  \terr1 : false (esc)
  \terr2 : false (esc)
  AT TIME 1
  \t_DELTA_ : (/ 1 6) (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : (/ 1 6) (esc)
  \tloc__AT0 : false (esc)
  \terr1 : false (esc)
  \terr2 : false (esc)
  AT TIME 2
  \t_DELTA_ : 0.0 (esc)
  \tx__AT0 : (/ 1 6) (esc)
  \ty__AT0 : (/ 1 3) (esc)
  \tloc__AT0 : false (esc)
  \terr1 : false (esc)
  \terr2 : true (esc)
Cex generation with non-strict delays:
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 -ta -p 1 --witness --strict-delays 0 samples/rtc/simple_ta_2.vmt
  Property 1 is FALSE
  sat
  AT TIME 0
  \t_DELTA_ : 0.0 (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : 0.0 (esc)
  \tloc__AT0 : true (esc)
  \terr1 : false (esc)
  \terr2 : false (esc)
  AT TIME 1
  \t_DELTA_ : 0.0 (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : 0.0 (esc)
  \tloc__AT0 : false (esc)
  \terr1 : false (esc)
  \terr2 : false (esc)
  AT TIME 2
  \t_DELTA_ : 0.0 (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : 0.0 (esc)
  \tloc__AT0 : false (esc)
  \terr1 : false (esc)
  \terr2 : true (esc)
Bounded model checking cannot conclude that a property holds
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 -ta -p 0 --witness samples/rtc/simple_ta.vmt
  Property 0 is UNKNOWN
  unknown
  [255]
But k-induction can:
  $ cd $TESTDIR/../.. && pono -e ind --smt-solver cvc5 -ta -p 0 --witness samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
IC3IA with external opensmt interpolator  
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 -ta --external-interpolator opensmt samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
IC3IA with external smtinterpol interpolator
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 -ta --external-interpolator smtinterpol samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
Pono rejects timed automaton files with properties that contain clock constraints in the (default) delay-first semantics:
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 -ta -p 0 --witness --delay-first 1 samples/rtc/simple_ta_3.vmt  
  Properties cannot contain clock constraints when using the delay-first timed automata semantics. See property 0.
  error
  b0
  [2]
However, the same file is accepted in the delay-second semantics:
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 -ta -p 0 --witness --delay-first 0 samples/rtc/simple_ta_3.vmt
  Property 0 is FALSE
  sat
  AT TIME 0
  \t_DELTA_ : 7.0 (esc)
  \tx__AT0 : 0.0 (esc)
  \ty__AT0 : 0.0 (esc)
  \tloc__AT0 : true (esc)
  AT TIME 1
  \t_DELTA_ : 0.0 (esc)
  \tx__AT0 : 7.0 (esc)
  \ty__AT0 : 7.0 (esc)
  \tloc__AT0 : true (esc)
RT-Consistency checking with BMC (with a quantified property)
  $ cd $TESTDIR/../.. && pono -e bmc --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
  Property 0 is UNKNOWN
  unknown
  [255]
RT-Consistency checking with k-induction (with a quantified property) - this is less efficient but works
  $ cd $TESTDIR/../.. && pono -e ind --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
A non-rt-consistent example
  $ cd $TESTDIR/../.. && pono -e ind --smt-solver cvc5 --rt-consistency 1 --witness samples/rtc/sample_inconsistent.smv
  Property 0 is FALSE
  sat
  AT TIME 0
  \ti : true (esc)
  \tclk : false (esc)
  \tv : false (esc)
  \tstate : false (esc)
  AT TIME 1
  \ti : false (esc)
  \tclk : false (esc)
  \tv : false (esc)
  \tstate : true (esc)
RT-Consistency checking with quanfier elimination and ic3ia; thus reduction to safety checking
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 --external-interpolator opensmt samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with quanfier elimination and ic3ia; rt-inconsistent example
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 --witness --external-interpolator opensmt samples/rtc/sample_inconsistent.smv
  Property 0 is FALSE
  sat
  AT TIME 0
  \ti : true (esc)
  \tstate : false (esc)
  \tv : false (esc)
  \tclk : false (esc)
  AT TIME 1
  \ti : false (esc)
  \tstate : true (esc)
  \tv : false (esc)
  \tclk : false (esc)
RT-Consistency checking with ic3ia and quantified property
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 --external-interpolator opensmt samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with ic3ia and quantified property (inconsistent case)
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 --witness --external-interpolator opensmt samples/rtc/sample_inconsistent.smv
  Property 0 is FALSE
  sat
  AT TIME 0
  \ti : true (esc)
  \tstate : false (esc)
  \tv : false (esc)
  \tclk : false (esc)
  AT TIME 1
  \ti : false (esc)
  \tstate : true (esc)
  \tv : false (esc)
  \tclk : false (esc)
RT-Consistency checking with ic3ia; case of timed automata
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 1 --external-interpolator opensmt samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with ic3ia; case of timed automata  
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 0 --external-interpolator opensmt samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
