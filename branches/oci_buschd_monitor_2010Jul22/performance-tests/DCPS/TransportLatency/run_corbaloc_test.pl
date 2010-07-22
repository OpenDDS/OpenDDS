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

$repo_bit_opt = "-ORBSvcConf tcp.conf";

if ($ARGV[0] eq 'udp') {
  $svc_conf = " -ORBSvcConf udp.conf ";
}
else {
    $svc_conf = " -ORBSvcConf tcp.conf";
}


my($port1) = PerlACE::random_port();
$dcpsrepo_ior = "repo.ior";
$common_args = "-DCPSInfoRepo corbaloc:iiop:localhost:$port1/DCPSInfoRepo"
    . " $svc_conf";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "$repo_bit_opt -o $dcpsrepo_ior "
                                . "-ORBEndpoint iiop://localhost:$port1");
$Subscriber = PerlDDS::create_process ("subscriber",
                                    "-DCPSConfigFile sub.ini $common_args");
$Publisher = PerlDDS::create_process ("publisher",
                                   "-DCPSConfigFile pub.ini $common_args");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}
unlink $dcpsrepo_ior;

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


if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
