eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../FooType');

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");


$DCPSREPO = new PerlACE::Process ("../../../../DDS/DCPSInfoRepo",
                            "-o $dcpsrepo_ior"
                            . " -d $domains_file -ORBDebugLevel 1");


$FooTest = new PerlACE::Process ("SimpleFooTest",
                            "-DCPSInfoRepo file://$dcpsrepo_ior");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 5) == -1) {
    print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
    exit 1;
}


$FooTest->Spawn ();
sleep 5;
$FooResult = $FooTest->WaitKill (30);

if ($FooResult != 0) {
    print STDERR "ERROR: SimpleFooTest returned $FooResult\n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(20);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

exit $status;
