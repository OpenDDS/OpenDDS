eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT @LD_LIBRARY_PATH);
BEGIN { unshift @LD_LIBRARY_PATH, "../FooType"; }
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

$status = 0;

$opts = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf tcp.conf";
$pub_opts = "$opts -s 1 -t 100";
$sub_opts = "$opts -n 100";

$dcpsrepo_ior = "repo.ior";
$repo_bit_opt = $opts;

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$repo_bit_opt -o $dcpsrepo_ior ");
$Subscriber = PerlDDS::create_process ("subscriber", "$sub_opts");
$Publisher = PerlDDS::create_process ("publisher", "$pub_opts ");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$Subscriber->Spawn ();
$Publisher->Spawn ();

$SubscriberResult = $Subscriber->WaitKill (300);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
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
