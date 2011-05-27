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

$svc_conf = '';

if (!new PerlACE::ConfigList->check_config ('STATIC')) {
    if ($ARGV[0] eq 'udp') {
        $svc_conf = " -ORBSvcConf udp.conf ";
    }
    else {
        $svc_conf = " -ORBSvcConf ../../tcp.conf";
    }
}

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$svc_conf -NOBITS -o $dcpsrepo_ior ");
$Subscriber = PerlDDS::create_process ("subscriber", "$svc_conf -DCPSBit 0 -DCPSConfigFile sub.ini");
$Publisher = PerlDDS::create_process ("publisher", "$svc_conf -DCPSBit 0 -DCPSConfigFile pub.ini");

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
