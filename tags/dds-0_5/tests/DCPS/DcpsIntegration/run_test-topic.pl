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

PerlACE::add_lib_path('../FooType');


$DCPSREPO = new PerlACE::Process ("../../../dds/InfoRepo/DCPSInfoRepo",
                            "-o $dcpsrepo_ior"
                            . " -d $domains_file");


$Topic = new PerlACE::Process ("topic_test",
                               "-DCPSInfo file://$dcpsrepo_ior");

$DCPSREPO->Spawn ();
sleep 5;

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

exit $status;
