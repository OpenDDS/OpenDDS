eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../FooType4');
PerlACE::add_lib_path('../common');

# single reader with single instances test
$use_take = 0;
$use_udp = 0;
$sub_addr = "localhost:16701";
$pub_addr = "localhost:";
$port=29804;
$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');
$svc_conf = $use_svc_conf ? " -ORBSvcConf ../../tcp.conf " : '';

$arg_idx = 0;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
$repo_bit_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf" : '';

unlink $dcpsrepo_ior; 

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf "
#                           . "-ORBDebugLevel 1 "
                           . "-o $dcpsrepo_ior "
                           . "-d $domains_file ");


print $DCPSREPO->CommandLine(), "\n";
              
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
$threshold_liveliness_lost = ($overlap_time / $sub_lease_time) * 0.7;
$sub_parameters = "$svc_conf -s $sub_addr -t $threshold_liveliness_lost -l $sub_lease_time -x $sub_time -ORBDebugLevel $level";

$Subscriber = new PerlACE::Process ("subscriber", $sub_parameters);
print $Subscriber->CommandLine(), "\n";

$pub_parameters = "$svc_conf $common_parameters" ;

for($i = 0; $i < $numPubs; ++$i)
{
  $thisPort = $port + $i;
  $thisPubTime = $pub_time - ($i * $delay);
  $thisPubLeaseTime = $pub_lease_time;
  if($i == 0) {
    # one publisher will have a bad lease time
    $thisPubLeaseTime = $sub_lease_time * 1.5;
  }
  $thePublisher = new PerlACE::Process ("publisher", "$pub_parameters -l $thisPubLeaseTime -x $thisPubTime -ORBDebugLevel $level -p $pub_addr$thisPort");
  push @Publisher, $thePublisher;
  print $thePublisher->CommandLine(), "\n";
}

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$SubscriberResult = $Subscriber->Spawn();

if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

sleep 10;

foreach $pub (@Publisher)
{
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
