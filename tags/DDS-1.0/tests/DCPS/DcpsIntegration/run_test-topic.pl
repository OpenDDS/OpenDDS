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
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");
$bit_conf = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf ../../tcp.conf";

unlink $dcpsrepo_ior;

PerlACE::add_lib_path('../FooType');


$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                            "$bit_conf  -o $dcpsrepo_ior"
                            . " -d $domains_file");


$Topic = new PerlACE::Process ("topic_test",
                               "$bit_conf  -DCPSInfoRepo file://$dcpsrepo_ior");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 5) == -1) {
    print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
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
