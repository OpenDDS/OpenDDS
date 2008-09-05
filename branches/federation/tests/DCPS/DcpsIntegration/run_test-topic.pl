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

$dcpsrepo_ior = "dcps_ir.ior";
$bit_conf = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf ../../tcp.conf";

unlink $dcpsrepo_ior;

PerlDDS::add_lib_path('../FooType');

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                              "$bit_conf  -o $dcpsrepo_ior ");


$Topic = PerlDDS::create_process ("topic_test",
                                 "$bit_conf  -DCPSInfoRepo file://$dcpsrepo_ior");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
    $DCPSREPO->Kill (); $DCPSREPO->TimedWait (1);
    exit 1;
}


$TopicResult = $Topic->SpawnWaitKill (60);

if ($TopicResult != 0) {
    print STDERR "ERROR: topic_test returned $TopicResult\n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(30);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

exit $status;
