#!/usr/bin/perl

use strict;
use warnings;

use Term::ANSIColor;

# Collect stack traces from autobuild's output.log
sub collect_stack_trace {
  my $logfile = shift;
  open(my $fh, $logfile) or die "Couldn't open file $logfile: $!\n";

  my $test_prefix = "auto_run_tests:";
  my $trace_begin = "Begin stack trace";
  my $trace_end = "End stack trace";
  my %traces;

  my $line = <$fh>;
  while (defined($line)) {
    my $idx = index($line, $test_prefix);
    if ($idx != -1) {
      my $test_name = substr($line, 16);

      while (defined($line = <$fh>) && index($line, $test_prefix) == -1) {
        if (index($line, $trace_begin) != -1) {
          my $trace = "";
          do {
            $trace = $trace . $line;
            $line = <$fh>;
          } while (index($line, $trace_end) == -1);
          $trace = $trace . $line;
          $traces{$test_name} = $trace;
        }
      }
    } else {
      $line = <$fh>;
    }
  }

  foreach my $test (keys %traces) {
    print color("red"), "$test", color("reset");
    print "$traces{$test}\n";
  }
  close($fh);
}

collect_stack_trace(shift);
