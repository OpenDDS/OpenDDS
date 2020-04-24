eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

$status = 0;


$pub_opts = "-DCPSConfigFile pub.ini -DCPSPersistentDataDir $DDS_ROOT/tests/DCPS/PersistentDurability/data";
$sub_opts = "-DCPSConfigFile sub.ini";

my $LONE_PROCESS = 1; #only one publisher process runs at a time

sub rmtree {
  # this invocation of the publisher just cleans up the durability files
  my $name = shift;
  my $Pub_delete = PerlDDS::create_process ("publisher", "$pub_opts -d $name", $LONE_PROCESS);
  $Pub_delete->SpawnWaitKill(60);
}

$dcpsrepo_ior = "repo.ior";
$repo_bit_opt = $opts;

unlink $dcpsrepo_ior;

$data_file = "test_run.data";
unlink $data_file;

$DCPSREPO =
  PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                           "-o $dcpsrepo_ior -DCPSDebugLevel 4");
$Subscriber = PerlDDS::create_process ("subscriber", "$sub_opts");
$Publisher1 = PerlDDS::create_process ("publisher",
                                       "$pub_opts -w -ORBLogFile $data_file",
                                       $LONE_PROCESS);
$Publisher2 = PerlDDS::create_process ("publisher",
                                       "$pub_opts -ORBLogFile $data_file",
                                       $LONE_PROCESS);

print $DCPSREPO->CommandLine() . "\n";
print $Publisher1->CommandLine() . "\n";
print $Publisher2->CommandLine() . "\n";
print $Subscriber->CommandLine() . "\n";

print "INFO: start Repo\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$durability_cache = 'data';
rmtree($durability_cache) if -d $durability_cache;

print "INFO: start publisher 1\n";
$Publisher1->Spawn ();

# Wait for the publisher to end before starting the subscriber so that
# the persistent data will be available to a newly spawned
# publisher.  This publisher will not wait for subscriptions.
$PublisherResult = $Publisher1->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher 1 returned $PublisherResult\n";
    $status = 1;
}
else {
    print "INFO: publisher 1 completed\n";
}

# Now spawn the publisher that will actually wait for the DataReader
# to complete its reads.
print "INFO: start publisher 2\n";
$Publisher2->Spawn ();

sleep (1);

print "INFO: start subscriber\n";
$Subscriber->Spawn ();

$PublisherResult = $Publisher2->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher 2 returned $PublisherResult\n";
    $status = 1;
}
else {
    print "INFO: publisher 2 completed\n";
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
    $status = 1;
}
else {
    print "INFO: subscriber  completed\n";
}

rmtree($durability_cache) if -d $durability_cache;

$ir = $DCPSREPO->TerminateWaitKill (5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;
unlink $data_file;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
