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

$status = 0;

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

# single reader with single instances test
$use_take = 0;
$use_udp = 0;
$sub_addr = "localhost:16701";
$pub_addr = "localhost:";
$port=29804;
$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');
$svc_conf = $use_svc_conf ? " -ORBSvcConf ../../tcp.conf " : '';

$arg_idx = 0;

$dcpsrepo_ior = "repo.ior";
$repo_bit_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf" : '';

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

unlink $dcpsrepo_ior; 

my $repoArgs = "$repo_bit_conf ";
$repoArgs .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoArgs .= "-DCPSTransportDebugLevel $repoTransportDebug " if $repoTransportDebug;
$repoArgs .= "-ORBLogFile $debugFile "     if $repoDebug and $debugFile;
$repoArgs .= "-o $dcpsrepo_ior ";
$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repoArgs);

# test multiple cases
$numPubs = 5;
$level = 0;
$delay = 3;
$overlap_time = 50;
$sub_time = $overlap_time + ($numPubs * $delay);
$pub_time = $sub_time + 20;
$pub_lease_time = 1;  # in msec
$sub_lease_time = $pub_lease_time * 2;
# this is the threshold number of publishers we would expect to fail the liveliness tests with a 70% fudge factor
$threshold_liveliness_lost = ($overlap_time / $sub_lease_time) * 0.6;

my $subArgs = "$svc_conf ";
$subArgs .= "-DCPSDebugLevel $subDebug " if $subDebug;
$subArgs .= "-DCPSTransportDebugLevel $subTransportDebug " if $subTransportDebug;
$subArgs .= "-ORBLogFile $debugFile "    if $subDebug and $debugFile;
$subArgs .= "-s $sub_addr -t $threshold_liveliness_lost -l $sub_lease_time -x $sub_time ";
$Subscriber = PerlDDS::create_process ("subscriber", $subArgs);

$pub_parameters = "$svc_conf $common_parameters" ;

for($i = 0; $i < $numPubs; ++$i)
{
  $thisPort = $port + $i;
  $thisPubTime = $pub_time - ($i * $delay);
  $thisPubLeaseTime = $pub_lease_time;
  $liveliness_factor = " ";
  if($i == 0) {
    # one publisher will have a bad lease time
    $factor = ($sub_lease_time / $pub_lease_time) * 1.5 * 100; # 100%
    $liveliness_factor = "-DCPSLivelinessFactor $factor ";
  }

  my $pubArgs = "$svc_conf ";
  $pubArgs .= "-DCPSDebugLevel $pubDebug " if $pubDebug;
  $pubArgs .= "-DCPSTransportDebugLevel $pubTransportDebug " if $pubTransportDebug;
  $pubArgs .= "-ORBLogFile $debugFile "    if $pubDebug and $debugFile;
  $pubArgs .= "-l $thisPubLeaseTime -x $thisPubTime -p $pub_addr$thisPort $liveliness_factor ";
  $thePublisher = PerlDDS::create_process ("publisher", $pubArgs);
  push @Publisher, $thePublisher;
}

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Subscriber->CommandLine() . "\n";
$SubscriberResult = $Subscriber->Spawn();

if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

sleep 10;

foreach $pub (@Publisher)
{
  print $pub->CommandLine() . "\n";
  $pub->Spawn ();
  sleep $delay;
}

$SubscriberResult = $Subscriber->WaitKill($sub_time);

if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

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
