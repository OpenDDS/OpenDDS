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


$status = 0;

$opts =
  new PerlACE::ConfigList->check_config ('STATIC')
  ? ''
  : "-ORBSvcConf tcp.conf";
$pub_opts = "$opts -DCPSConfigFile pub.ini";
$sub_opts = "$opts -DCPSConfigFile sub.ini";


sub rmtree {
  # this invocation of the publisher just cleans up the durability files
  my $name = shift;
  my $Pub_delete = PerlDDS::create_process ("publisher", "$pub_opts -d $name");
  $Pub_delete->SpawnWaitKill(60);
}

$dcpsrepo_ior = "repo.ior";
$repo_bit_opt = $opts;

unlink $dcpsrepo_ior;

$data_file = "test_run.data";
unlink $data_file;

$DCPSREPO =
  PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                           "$repo_bit_opt -o $dcpsrepo_ior ");
$Subscriber = PerlDDS::create_process ("subscriber", "$sub_opts");
$Publisher1 = PerlDDS::create_process ("publisher",
                                       "$pub_opts -w -ORBLogFile $data_file");
$Publisher2 = PerlDDS::create_process ("publisher",
                                       "$pub_opts -ORBLogFile $data_file");

print $DCPSREPO->CommandLine() . "\n";
print $Publisher1->CommandLine() . "\n";
print $Publisher2->CommandLine() . "\n";
print $Subscriber->CommandLine() . "\n";

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$durability_cache = "OpenDDS-durable-data-dir"; # Currently a fixed name
                                                # is used by OpenDDS.
rmtree($durability_cache) if -d $durability_cache;

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
$Publisher2->Spawn ();

sleep (1);

$Subscriber->Spawn ();

$PublisherResult = $Publisher2->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher 2 returned $PublisherResult\n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
    $status = 1;
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
