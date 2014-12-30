eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
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

my $numPubs = 5;
my $delay = 3;
my $overlap_time = 50;
my $sub_time = $overlap_time + ($numPubs * $delay);
my $pub_time = $sub_time + 20;
my $pub_lease_time = 1;  # in msec
my $sub_lease_time = $pub_lease_time * 2;
# this is the threshold number of publishers we would expect to fail the liveliness tests with a 70% fudge factor
my $threshold_liveliness_lost = ($overlap_time / $sub_lease_time) * 0.6;

my $subArgs = "";
$subArgs .= "-DCPSDebugLevel $subDebug " if $subDebug;
$subArgs .= "-DCPSTransportDebugLevel $subTransportDebug " if $subTransportDebug;
$subArgs .= "-ORBLogFile $debugFile "    if $subDebug and $debugFile;
$subArgs .= "-t $threshold_liveliness_lost -l $sub_lease_time -x $sub_time ";

$test->process('sub', 'subscriber', $subArgs);

for (my $i = 0; $i < $numPubs; ++$i) {
  my $thisPubTime = $pub_time - ($i * $delay);
  my $liveliness_factor;
  if ($i == 0) {
    # one publisher will have a bad lease time
    my $factor = ($sub_lease_time / $pub_lease_time) * 1.5 * 100; # 100%
    $liveliness_factor = "-DCPSLivelinessFactor $factor ";
  }

  my $pubArgs = "";
  $pubArgs .= "-DCPSDebugLevel $pubDebug " if $pubDebug;
  $pubArgs .= "-DCPSTransportDebugLevel $pubTransportDebug " if $pubTransportDebug;
  $pubArgs .= "-ORBLogFile $debugFile "    if $pubDebug and $debugFile;
  $pubArgs .= "-l $pub_lease_time -x $thisPubTime $liveliness_factor ";
  $test->process("pub$i", 'publisher', $pubArgs);
}

$test->start_process('sub');
sleep 10;

foreach my $p (0 .. $numPubs - 1) {
  $test->start_process("pub$p");
  sleep $delay;
}

exit $test->finish(65, 'sub');
