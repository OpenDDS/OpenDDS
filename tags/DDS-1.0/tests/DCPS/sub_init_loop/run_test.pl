eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
$subscriber_completed = PerlACE::LocalFile ("subscriber_finished.txt");
$subscriber_ready = PerlACE::LocalFile ("subscriber_ready.txt");
$publisher_ready = PerlACE::LocalFile ("publisher_ready.txt");
$testoutputfilename = PerlACE::LocalFile ("test.log");
my $common_opts = "";

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_ready;
unlink $testoutputfilename;

if ($#ARGV >= 0)
{
    if ($ARGV[0] == "verbose") {
	$common_opts = $common_opts." -v";
    }
}
$svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : " -ORBSvcConf ../../tcp.conf ";

$DCPSREPO = new PerlACE::Process
    ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo"
     , " $svc_config -o $dcpsrepo_ior"
     . " -d $domains_file -ORBSvcConf repo.conf");
$Subscriber = new PerlACE::Process
    ("subscriber"
     , " -v $svc_config -DCPSConfigFile sub.ini".$common_opts);
$Publisher = new PerlACE::Process
    ("publisher"
     , " $svc_config -DCPSConfigFile pub.ini".$common_opts);

print $DCPSREPO->CommandLine () . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Subscriber->CommandLine () . "\n";
$Subscriber->Spawn ();

print $Publisher->CommandLine () . "\n";
$Publisher->Spawn ();

$SubscriberResult = $Subscriber->WaitKill (300);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$PublisherResult = $Publisher->WaitKill (300);
if ($$PublisherResult != 0) {
    # writing out to STDOUT as these tests redirect STDERR to a log file.
    # The nightly script parses STDERR to detect test failures.
    print STDOUT "ERROR: Publisher crashed\n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_ready;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED. Please see the $testoutputfilename for details.\n";
}

exit $status;
