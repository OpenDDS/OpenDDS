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
use POSIX;

# exit status
$status = 0;

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

# subscriber address
$sub_addr = "localhost:16701";
# publisher address
$pub_addr = "localhost:";
# base port for publishers
$pub_port=29804;

# InfoRepo ior file
$dcpsrepo_ior = "repo.ior";

# debug settings
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

# setup the InfoRepo
unlink $dcpsrepo_ior;
my $repoArgs = "";
$repoArgs .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoArgs .= "-DCPSTransportDebugLevel $repoTransportDebug " if $repoTransportDebug;
$repoArgs .= "-ORBLogFile $debugFile "     if $repoDebug and $debugFile;
$repoArgs .= "-o $dcpsrepo_ior ";
$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repoArgs);

# test configuration
# The subscriber is started and allowed to run for $start_delay seconds.
# $numPubs publishers are started with a delay of $delay seconds.
# Once all the publishers are started, the subscriber runs for $overlap_time seconds.
$numPubs = 5;
$delay = 3;
$start_delay = 5;
$overlap_time = 50;
# The total time the subscriber runs.
$sub_time = $start_delay + ($numPubs - 1) * $delay + $overlap_time;
# The total time the publisher runs.
$pub_time = $sub_time;
# Liveliness lease times for the publisher and subscriber.
$pub_lease_time = 1; # sec
$sub_lease_time = 2; # sec
# The first publisher's liveliness factor is inflated causing it to lose liveliness.
$inflation_factor = 3;
if ($sub_least_time > $pub_lease_time * $inflation_factor) {
    print STDERR "ERROR:  Test misconfiguration.  Subscriber lease time too small.\n";
}
# Calculate the expected number of liveliness lost events.
# The - 1 handles corner cases.
$threshold_liveliness_lost = floor(($sub_time - $start_delay) / ($inflation_factor * $pub_lease_time)) - 1;

if ($threshold_liveliness_lost == 0) {
  print STDERR "ERROR:  Test misconfiguration.  Expected number of liveliness lost events is 0.\n";
  exit 1;
}

# setup the subscriber
my $subArgs = "";
$subArgs .= "-DCPSDebugLevel $subDebug " if $subDebug;
$subArgs .= "-DCPSTransportDebugLevel $subTransportDebug " if $subTransportDebug;
$subArgs .= "-ORBLogFile $debugFile "    if $subDebug and $debugFile;
$subArgs .= "-s $sub_addr -t $threshold_liveliness_lost -l $sub_lease_time -x $sub_time ";
$Subscriber = PerlDDS::create_process ("subscriber", $subArgs);

# setup the publishers
$pub_parameters = "$common_parameters" ;
for($i = 0; $i < $numPubs; ++$i)
{
  $thisPort = $pub_port + $i;
  $thisPubTime = $pub_time - ($i * $delay);
  $thisPubLeaseTime = $pub_lease_time;
  $factor = 10;
  if($i == 0) {
      $factor = $inflation_factor * 100;
  }
  $liveliness_factor = "-DCPSLivelinessFactor $factor ";

  my $pubArgs = "";
  $pubArgs .= "-DCPSDebugLevel $pubDebug " if $pubDebug;
  $pubArgs .= "-DCPSTransportDebugLevel $pubTransportDebug " if $pubTransportDebug;
  $pubArgs .= "-ORBLogFile $debugFile "    if $pubDebug and $debugFile;
  $pubArgs .= "-l $thisPubLeaseTime -x $thisPubTime -p $pub_addr$thisPort $liveliness_factor ";
  $thePublisher = PerlDDS::create_process ("publisher", $pubArgs);
  push @Publisher, $thePublisher;
}

# start the InfoRepo
print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

# start the subscriber
print $Subscriber->CommandLine() . "\n";
$SubscriberResult = $Subscriber->Spawn();
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

# Let the subscriber settle(?).
sleep $start_delay;

# start the publishers
foreach $pub (@Publisher)
{
  print $pub->CommandLine() . "\n";
  $pub->Spawn ();
  sleep $delay;
}

# stop the subscriber
$SubscriberResult = $Subscriber->WaitKill($sub_time);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

# stop the publishers
$pubNum = 0;
foreach $pub (@Publisher)
{
  $pubNum++;
  $PublisherResult = $pub->WaitKill ($pub_time * 3);

  if ($PublisherResult != 0) {
      print STDERR "ERROR: publisher$pubNum returned $PublisherResult \n";
      $status = 1;
  }
}

# stop the InfoRepo
$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
