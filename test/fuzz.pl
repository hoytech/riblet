#!/usr/bin/env perl

use strict;

my $runs = $ENV{RUNS} // 10;
my $numRecs = $ENV{NUM} // 10000;
my $seed = $ENV{SEED} // 1;

my $p1 = $ENV{P1} // 0.1;
my $p2 = $ENV{P2} // 0.1;

my $riblet = $ENV{RIBLET_BIN} || '../riblet';
my $tmpDir = './test-riblet-tmp/';


my $numCoded = $numRecs * 2;


die "bad P1" if $p1 < 0 || $p1 > 1;
die "bad P2" if $p2 < 0 || $p2 > 1;
my $p3 = 1 - ($p1 + $p2);
die "P1 and P2 must sum to <= 1" if ($p1 + $p2) > 1;


for (1..$runs) {
    doFuzz();
}


sub doFuzz {
    mkdir("$tmpDir/");
    sys("rm -f $tmpDir/*.txt $tmpDir/*.riblet");

    my $s1 = {};
    my $s2 = {};

    print "-------- SEED = $seed -----------\n";
    srand($seed);
    $seed++;

    for my $i (1..$numRecs) {
        my $rec = rand() * 1e9;

        my $p = rand();

        if ($p < $p1) {
            # only in s1
            $s1->{$rec} = 1;
        } elsif ($p < ($p1 + $p2)) {
            # only in s2
            $s2->{$rec} = 1;
        } else {
            # in both
            $s1->{$rec} = 1;
            $s2->{$rec} = 1;
        }
    }

    {
        open(my $fh, ">", "$tmpDir/s1.txt") || die;
        for my $k (keys %$s1) {
            print $fh "$k\n";
        }
    }

    {
        open(my $fh, ">", "$tmpDir/s2.txt") || die;
        for my $k (keys %$s2) {
            print $fh "$k\n";
        }
    }

    sys("$riblet build --num $numCoded $tmpDir/s1.txt");
    sys("$riblet build --num $numCoded $tmpDir/s2.txt");

    sys("$riblet diff --sym $tmpDir/s1.txt.riblet $tmpDir/s2.txt.riblet > $tmpDir/diff");

    sys(qq{bash -c "diff <(sort $tmpDir/s1.txt) <(sort $tmpDir/s2.txt $tmpDir/diff | uniq -u) > $tmpDir/delta1"});
    sys(qq{bash -c "diff <(sort $tmpDir/s2.txt) <(sort $tmpDir/s1.txt $tmpDir/diff | uniq -u) > $tmpDir/delta2"});

    die "difference detected in delta1" if -s "$tmpDir/delta1";
    die "difference detected in delta2" if -s "$tmpDir/delta2";
}


sub sys {
    my $cmd = shift;
    print "SYS: $cmd\n";
    system($cmd) && die "system failed";
}
