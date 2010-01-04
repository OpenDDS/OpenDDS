eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

$send_interval = 0;
$test_duration = 300;
if ($ARGV[0] eq 'long') {
  $send_interval = 30;
  $test_duration = 600;
}

$status = 0;
$use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');

$opts = $use_svc_config ? "-ORBSvcConf tcp.conf" : '';
$repo_bit_opt = $opts;

$pub_opts = "$opts -i $send_interval -ORBDebugLevel 10 -ORBLogFile pub.log -DCPSConfigFile pub.ini -DCPSDebugLevel 10";
$sub_opts = "$opts -DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile sub.log -DCPSConfigFile sub.ini -DCPSDebugLevel 10";
$mon_opts = "$opts -DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile mon.log -DCPSConfigFile sub.ini -DCPSDebugLevel 10";

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-DCPSDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

$Monitor    = PerlDDS::create_process ("monitor",    " $mon_opts");
$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
$Publisher  = PerlDDS::create_process ("publisher",  " $pub_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Monitor->CommandLine() . "\n";
$Monitor->Spawn ();

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


$PublisherResult = $Publisher->WaitKill ($test_duration);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$MonitorResult = $Monitor->TerminateWaitKill(5);
if ($MonitorResult != 0) {
    print STDERR "ERROR: Monitor returned $MonitorResult\n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
