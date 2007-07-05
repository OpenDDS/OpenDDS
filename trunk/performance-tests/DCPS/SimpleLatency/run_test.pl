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
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-ORBSvcConf ../../tcp.conf -DCPSBit 0";

unlink $dcpsrepo_ior;

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
				  "$repo_bit_conf -o $dcpsrepo_ior -d $domains_file");

$Subscriber = new PerlACE::Process ("dds_sub", "$app_bit_conf");

$Publisher = new PerlACE::Process ("dds_pub", "$app_bit_conf -s 200 -c 10000");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$Publisher->Spawn ();

$Subscriber->Spawn ();


$PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
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
