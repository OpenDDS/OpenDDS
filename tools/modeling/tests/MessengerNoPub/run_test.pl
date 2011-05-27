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

$sub_opts = "$opts -DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile subscriber.log -DCPSDebugLevel 10";

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

PerlDDS::add_lib_path("./model");

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

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

exit $status;
