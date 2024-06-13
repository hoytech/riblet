#!/usr/bin/env perl

use strict;

# Edge cases

sys("RUNS=1 P1=1 P2=0 ./fuzz.pl");
sys("RUNS=1 P1=0 P2=1 ./fuzz.pl");
sys("RUNS=1 P1=0 P2=0 ./fuzz.pl");

# Different distributions

sys("RUNS=5 P1=0.5 P2=0.1 ./fuzz.pl");
sys("RUNS=5 P1=0.1 P2=0.5 ./fuzz.pl");
sys("RUNS=5 P1=0.01 P2=0.01 ./fuzz.pl");
sys("RUNS=5 P1=0.45 P2=0.45 ./fuzz.pl");

# Bigger and bigger sizes

sys("NUM=10000 RUNS=2 ./fuzz.pl");
sys("NUM=100000 RUNS=2 ./fuzz.pl");
sys("NUM=1000000 RUNS=2 ./fuzz.pl");


sub sys {
    my $cmd = shift;
    print "SYS: $cmd\n";
    system($cmd) && die "system failed";
}
