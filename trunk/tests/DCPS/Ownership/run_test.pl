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
$use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');

$opts = $use_svc_config ? "-ORBSvcConf tcp.conf" : '';
$repo_bit_opt = $opts;

$debuglevel = 0;

$pub_opts = "$opts -ORBDebugLevel $debuglevel -DCPSConfigFile pub.ini -DCPSDebugLevel $debuglevel";
$sub_opts = "$opts -DCPSTransportDebugLevel $debuglevel -ORBDebugLevel $debuglevel -DCPSConfigFile sub.ini -DCPSDebugLevel $debuglevel";
$shutdown_pub = 0;
$sub_deadline = "";
$pub1_deadline = "";
$pub2_deadline = "";
$pub1_reset_strength = "";
$sub_liveliness = "";
$pub1_liveliness = "";
$pub2_liveliness = "";
$testcase = 0;

if ($ARGV[0] eq 'liveliness_change') {
    $sub_liveliness = "-l 2";
    $pub1_liveliness = "-l 1";
    $pub2_liveliness = "-l 1";
    $testcase = 1;
}
elsif ($ARGV[0] eq 'miss_deadline') {
    $sub_deadline = "-d 2";
    $pub1_deadline = "-d 1";
    $pub2_deadline = "-d 1";
    $testcase = 2;
}
elsif ($ARGV[0] eq 'update_strength') {
    $pub1_reset_strength = "-r 15";
    $testcase = 3;
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-ORBDebugLevel 10 -ORBVerboseLogging 1 -DCPSDebugLevel $debuglevel -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts -DCPSDebugLevel $debuglevel -ORBVerboseLogging 1 -ORBLogFile sub.log $sub_deadline $sub_liveliness -t $testcase");

$Publisher1 = PerlDDS::create_process ("publisher", " $pub_opts -s 10 -i datawriter1 $pub1_reset_strength $pub1_deadline $pub1_liveliness -ORBLogFile pub1.log");
$Publisher2 = PerlDDS::create_process ("publisher", " $pub_opts -s 12 -i datawriter2 $pub2_deadline $pub2_liveliness -ORBLogFile pub2.log");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Publisher1->CommandLine() . "\n";
$Publisher1->Spawn ();


print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


print $Publisher2->CommandLine() . "\n";
$Publisher2->Spawn ();


$PublisherResult = $Publisher1->WaitKill (60);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}


$PublisherResult = $Publisher2->WaitKill (60);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}


$SubscriberResult = $Subscriber->WaitKill (60);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
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
