eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (ACE_ROOT, DDS_ROOT);
use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use PerlDDS::Run_Test;
use strict;
use warnings;

my $trials = 1;
for my $arg (@ARGV) {
  if ($arg =~ /^trials=(\d+)$/ && $1 > 0) {
    $trials = $1;
  } else {
    die "Usage: $0 [trials=<positive integer>]\n";
  }
}

my $failed = 0;
for my $trial (1 .. $trials) {
  print "ShmemWakeupRace trial $trial of $trials\n";

  my $test = new PerlDDS::TestFramework();
  $test->process('subscriber', 'subscriber',
                 '-DCPSConfigFile shmem_rtps.ini -DCPSDebugLevel 0 -DCPSTransportDebugLevel 0');
  $test->process('publisher', 'publisher',
                 '-DCPSConfigFile shmem_rtps.ini -DCPSDebugLevel 0 -DCPSTransportDebugLevel 0');

  $test->start_process('subscriber');
  $test->start_process('publisher');

  if ($test->finish(15)) {
    ++$failed;
  }
}

print "ShmemWakeupRace failed trials: $failed/$trials\n";
exit($failed ? 1 : 0);
