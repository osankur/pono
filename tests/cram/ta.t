Run these tests with
PONO=/home/osankur/tools/pono cram -i ta.t
Testing simple reachability queries  
  $ pono -e bmc --smt-solver cvc5 -ta -p 0 --witness $PONO/samples/rtc/simple_ta_2.vmt
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
  $ pono -e bmc --smt-solver cvc5 -ta -p 1 --witness $PONO/samples/rtc/simple_ta_2.vmt
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
  $ pono -e bmc --smt-solver cvc5 -ta -p 1 --witness --strict-delays 0 $PONO/samples/rtc/simple_ta_2.vmt
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
  $ pono -e bmc --smt-solver cvc5 -ta -p 0 --witness $PONO/samples/rtc/simple_ta.vmt
  Property 0 is UNKNOWN
  unknown
  [255]
But k-induction can:
  $ pono -e ind --smt-solver cvc5 -ta -p 0 --witness $PONO/samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
IC3IA with msat interpolator (msat must be installed)
  $ pono -e ic3ia --smt-solver msat -ta -p 0 --witness $PONO/samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
IC3IA with external opensmt interpolator  
  $ pono -e ic3ia --smt-solver cvc5 -ta --external-interpolator opensmt $PONO/samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
IC3IA with external smtinterpol interpolator
  $ pono -e ic3ia --smt-solver cvc5 -ta --external-interpolator smtinterpol $PONO/samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
Pono rejects timed automaton files with properties that contain clock constraints in the (default) delay-first semantics:
  $ pono -e bmc --smt-solver cvc5 -ta -p 0 --witness --delay-first 1 $PONO/samples/rtc/simple_ta_4.vmt  
  terminate called after throwing an instance of 'PonoException'
    what():  Properties cannot contain clock constraints when using the delay-first timed automata semantics. See property 0
  Aborted (core dumped)
  [134]
However, the same file is accepted in the delay-second semantics:
  $ pono -e bmc --smt-solver cvc5 -ta -p 0 --witness --delay-first 0 $PONO/samples/rtc/simple_ta_4.vmt
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
  $ pono -e bmc --smt-solver cvc5 --rt-consistency 1 $PONO/samples/rtc/sample_consistent.smv
  Property 0 is UNKNOWN
  unknown
  [255]
RT-Consistency checking with k-induction (with a quantified property) - this is less efficient but works
  $ pono -e ind --smt-solver cvc5 --rt-consistency 1 $PONO/samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
A non-rt-consistent example
  $ pono -e ind --smt-solver cvc5 --rt-consistency 1 --witness $PONO/samples/rtc/sample_inconsistent.smv
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
  $ pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 $PONO/samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with quanfier elimination and ic3ia; rt-inconsistent example
  $ pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 --witness $PONO/samples/rtc/sample_inconsistent.smv
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
  $ pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 $PONO/samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with ic3ia and quantified property (inconsistent case)
  $ pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 --witness $PONO/samples/rtc/sample_inconsistent.smv
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
  $ pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 1 $PONO/samples/rtc/simple_ta.vmt
  number nb: 3
  \terror: 0 (esc)
  \terror: 0 (esc)
  number nb: 3
  \terror: 0 (esc)
  \terror: 0 (esc)
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with ic3ia; case of timed automata  
  $ pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 0 $PONO/samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
