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

#Clean the Foo.txt file which is used as a storage of the 
#data written.

unlink "Foo.txt";

$status = 0;

PerlDDS::add_lib_path('../FooType2');

$num_threads_to_write=5;
$dcpsrepo_ior = "dcps_ir.ior";

unlink $dcpsrepo_ior; 

$DCPSREPO = PerlDDS::create_process ("../../../../DDS/DCPSInfoRepo",
                               "-o $dcpsrepo_ior"
                               . " -ORBDebugLevel 1");

#Test with multiple write threads and non blocking write.
$FooTest_1 = PerlDDS::create_process ("FooTest2",
                              "-DCPSInfoRepo file://$dcpsrepo_ior "
                              ."-t $num_threads_to_write");

#Test write block waiting for available space with RELIABLE and 
#KEEP_ALL qos.
$FooTest_2 = PerlDDS::create_process ("FooTest2",
                              "-DCPSInfoRepo file://$dcpsrepo_ior "
                              ."-t $num_threads_to_write -b 1");

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$FooTest_1->Spawn ();

$FooResult = $FooTest_1->WaitKill (60);

if ($FooResult != 0) {
    print STDERR "ERROR: FooTest_1 returned $FooResult\n";
    $status = 1;
}

unlink "Foo.txt";

$FooTest_2->Spawn ();

$FooResult = $FooTest_2->WaitKill (60);

if ($FooResult != 0) {
    print STDERR "ERROR: FooTest_2 returned $FooResult\n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(30);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

exit $status;
