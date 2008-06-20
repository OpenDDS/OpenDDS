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

$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');

$opts = $use_svc_conf ? " -ORBSvcConf ../../tcp.conf " : '';
$pub_opts = "$opts -DCPSConfigFile pub.ini -DCPSBit 0 -t5 -n5 -p5 -s5";
$sub_opts = "$opts -DCPSConfigFile sub.ini -DCPSBit 0 -t5 -n5 -s5 -p10";

$domains_file = "domain_ids";
$dcpsrepo_ior = "repo.ior";
$sync_ior="sync.ior";
$repo_bit_opt = "$opts -NOBITS";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
				  "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file");
$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
$Publisher = PerlDDS::create_process ("publisher", " $pub_opts");
$Publisher2 = PerlDDS::create_process ("publisher", " $pub_opts"." -i2");
$SyncServer = PerlDDS::create_process ("syncServer"
				    , "-p2 -s1");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $SyncServer->CommandLine() . "\n";
$SyncServer->Spawn ();
if (PerlACE::waitforfile_timed ($sync_ior, 10) == -1) {
    print STDERR "ERROR: waiting for Sync IOR file\n";
    $DCPSREPO->Kill ();
    $SyncServer->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();
print $Publisher2->CommandLine() . "\n";
$Publisher2->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

$PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}
$PublisherResult2 = $Publisher2->WaitKill (300);
if ($PublisherResult2 != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult2 \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$ss = $SyncServer->WaitKill(5);
if ($ss != 0) {
    print STDERR "ERROR: SyncServer returned $ss\n";
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
