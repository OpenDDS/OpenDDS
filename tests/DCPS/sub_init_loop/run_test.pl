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

$dcpsrepo_ior = "repo.ior";
$subscriber_completed = "subscriber_finished.txt";
$subscriber_ready = "subscriber_ready.txt";
$publisher_ready = "publisher_ready.txt";
$testoutputfilename = "test.log";
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

$DCPSREPO = PerlDDS::create_process
      ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo"
       , "-o $dcpsrepo_ior"
       . " -ORBSvcConf repo.conf");
$Subscriber = PerlDDS::create_process
      ("subscriber"
       , " -v -DCPSConfigFile sub.ini".$common_opts);
$Publisher = PerlDDS::create_process
      ("publisher"
       , "-DCPSConfigFile pub.ini".$common_opts);

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
