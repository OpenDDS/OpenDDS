eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;
$debuglevel = 0;

$pub_opts = "-ORBDebugLevel $debuglevel -DCPSConfigFile pub.ini -DCPSDebugLevel $debuglevel -DCPSBits 0";
$sub_opts = "-DCPSTransportDebugLevel $debuglevel -ORBDebugLevel $debuglevel -DCPSConfigFile sub.ini -DCPSDebugLevel $debuglevel -DCPSBits 0";
$testcase = "";

if ($ARGV[0] eq 'group') {
  $testcase = "-q 2"; #default
}
elsif ($ARGV[0] eq 'topic') {
  $testcase = "-q 1";
}
elsif ($ARGV[0] eq 'instance') {
  $testcase = "-q 0";
}
elsif ($ARGV[0] ne '') {
  print STDERR "ERROR: invalid parameter $ARGV[0]\n";
  exit 1;
}


$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                     "-ORBDebugLevel 10 -ORBVerboseLogging 1 -DCPSDebugLevel $debuglevel -ORBLogFile DCPSInfoRepo.log -o $dcpsrepo_ior -NOBITS");

$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts -ORBVerboseLogging 1 $testcase");

$Publisher = PerlDDS::create_process ("publisher", " $pub_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();


print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


$PublisherResult = $Publisher->WaitKill (60);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult\n";
    $status = 1;
}


$SubscriberResult = $Subscriber->WaitKill (60);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
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
