eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use POSIX qw(floor);
use strict;

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

my $debug ;# = 10;
my $repoDebug;
my $subDebug;
my $pubDebug;
my $debugFile;
$repoDebug = $debug if not $repoDebug and $debug;
$subDebug  = $debug if not $subDebug  and $debug;
$pubDebug  = $debug if not $pubDebug  and $debug;

my $transportDebug;
my $repoTransportDebug;
my $subTransportDebug ;# = 10;
my $pubTransportDebug;
$repoTransportDebug = $transportDebug if not $repoTransportDebug and $transportDebug;
$subTransportDebug  = $transportDebug if not $subTransportDebug  and $transportDebug;
$pubTransportDebug  = $transportDebug if not $pubTransportDebug  and $transportDebug;

my $test = new PerlDDS::TestFramework();
$test->{'wait_after_first_proc'} = 50;
$test->enable_console_logging();

my $repoArgs = "";
$repoArgs .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoArgs .= "-DCPSTransportDebugLevel $repoTransportDebug " if $repoTransportDebug;
$repoArgs .= "-ORBLogFile $debugFile "     if $repoDebug and $debugFile;
$test->setup_discovery($repoArgs);

# test configuration
# The subscriber is started and allowed to run for $start_delay seconds.
# $numPubs publishers are started with a delay of $delay seconds.
# Once all the publishers are started, the subscriber runs for $overlap_time seconds.
my $numPubs = 5;
my $delay = 3;
my $start_delay = 5;
my $overlap_time = 50;
# The total time the subscriber runs.
my $sub_time = $start_delay + ($numPubs - 1) * $delay + $overlap_time;
# The total time the publisher runs.
my $pub_time = $sub_time;
# Liveliness lease times for the publisher and subscriber.
my $pub_lease_time = 1; # sec
my $sub_lease_time = 2; # sec
# The first publisher's liveliness factor is inflated causing it to lose liveliness.
my $inflation_factor = 3;
if ($sub_lease_time > $pub_lease_time * $inflation_factor) {
  print STDERR "ERROR:  Test misconfiguration.  Subscriber lease time too small.\n";
}
# Calculate the expected number of liveliness lost events.
# The - 1 handles corner cases.
my $threshold_liveliness_lost = floor(($sub_time - $start_delay) / ($inflation_factor * $pub_lease_time)) - 1;

if ($threshold_liveliness_lost == 0) {
  print STDERR "ERROR:  Test misconfiguration.  Expected number of liveliness lost events is 0.\n";
  exit 1;
}

my $subArgs = "";
$subArgs .= "-DCPSDebugLevel $subDebug " if $subDebug;
$subArgs .= "-DCPSTransportDebugLevel $subTransportDebug " if $subTransportDebug;
$subArgs .= "-ORBLogFile $debugFile "    if $subDebug and $debugFile;
$subArgs .= "-t $threshold_liveliness_lost -l $sub_lease_time -x $sub_time";
$test->process('sub', 'subscriber', $subArgs);

for (my $i = 0; $i < $numPubs; ++$i) {
  my $thisPubTime = $pub_time - ($i * $delay);
  my $thisPubLeaseTime = $pub_lease_time;
  my $factor = 80;
  if ($i == 0) {
    $factor = $inflation_factor * 100;
  }
  my $liveliness_factor = "-DCPSLivelinessFactor $factor ";

  my $pubArgs = "";
  $pubArgs .= "-DCPSDebugLevel $pubDebug " if $pubDebug;
  $pubArgs .= "-DCPSTransportDebugLevel $pubTransportDebug " if $pubTransportDebug;
  $pubArgs .= "-ORBLogFile $debugFile "    if $pubDebug and $debugFile;
  $pubArgs .= "-l $thisPubLeaseTime -x $thisPubTime $liveliness_factor";
  $test->process("pub$i", 'publisher', $pubArgs);
}

$test->start_process('sub');

# Let the subscriber settle(?).
sleep $start_delay;

foreach my $p (0 .. $numPubs - 1) {
  $test->start_process("pub$p");
  sleep $delay;
}

exit $test->finish(65, 'sub');
