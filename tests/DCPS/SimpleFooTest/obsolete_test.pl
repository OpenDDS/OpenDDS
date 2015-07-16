eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

PerlDDS::add_lib_path('../FooType');

$dcpsrepo_ior = "dcps_ir.ior";

$DCPSREPO = PerlDDS::create_process ("$DDS_ROOT/bin/DCPSInfoRepo",
                              "-o $dcpsrepo_ior"
                              . " -ORBDebugLevel 1");


$FooTest = PerlDDS::create_process ("SimpleFooTest",
                              "-DCPSInfoRepo file://$dcpsrepo_ior");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
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
