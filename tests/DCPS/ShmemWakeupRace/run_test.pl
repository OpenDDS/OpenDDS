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
my $transport = 'shmem';
for my $arg (@ARGV) {
  if ($arg =~ /^trials=(\d+)$/ && $1 > 0) {
    $trials = $1;
  } elsif ($arg =~ /^transport=(shmem|rtps_udp)$/) {
    $transport = $1;
  } else {
    die "Usage: $0 [trials=<positive integer>] [transport=shmem|rtps_udp]\n";
  }
}

my $config = $transport eq 'shmem' ? 'shmem_rtps.ini' : 'rtps_udp.ini';
my $failed = 0;
for my $trial (1 .. $trials) {
  print "ShmemWakeupRace transport=$transport trial $trial of $trials\n";

  my $test = new PerlDDS::TestFramework();
  $test->process('subscriber', 'subscriber',
                 "-DCPSConfigFile $config -DCPSDebugLevel 0 -DCPSTransportDebugLevel 0");
  $test->process('publisher', 'publisher',
                 "-DCPSConfigFile $config -DCPSDebugLevel 0 -DCPSTransportDebugLevel 0");

  $test->start_process('subscriber');
  $test->start_process('publisher');

  if ($test->finish(20)) {
    ++$failed;
  }
}

print "ShmemWakeupRace transport=$transport failed trials: $failed/$trials\n";
exit($failed ? 1 : 0);
