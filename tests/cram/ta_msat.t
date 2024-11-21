IC3IA with msat interpolator (msat must be installed)
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver msat -ta -p 0 --witness samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with quanfier elimination and ic3ia; thus reduction to safety checking
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with quanfier elimination and ic3ia; rt-inconsistent example
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 0 --witness samples/rtc/sample_inconsistent.smv
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
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 samples/rtc/sample_consistent.smv
  Property 0 is TRUE
  unsat
  [1]
RT-Consistency checking with ic3ia and quantified property (inconsistent case)
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 --rt-consistency 1 --witness samples/rtc/sample_inconsistent.smv
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
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 1 samples/rtc/simple_ta.vmt
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
  $ cd $TESTDIR/../.. && pono -e ic3ia --smt-solver cvc5 -ta --rt-consistency 0 samples/rtc/simple_ta.vmt
  Property 0 is TRUE
  unsat
  [1]
