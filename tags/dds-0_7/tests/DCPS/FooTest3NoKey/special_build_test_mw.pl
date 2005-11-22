eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

# Set the library path for the client to be able to load
# the FooTyoe* library.
PerlACE::add_lib_path('../FooType3NoKey');

#Clean the Foo.txt file which is used as a storage of the 
#data written.

unlink "Foo.txt";

$status = 0;

$num_threads_to_write=5;
$num_writes_per_thread=2;
$num_writers=2;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");

unlink $dcpsrepo_ior; 

$DCPSREPO = new PerlACE::Process ("../../../../DDS/DCPSInfoRepo",
                             "-o $dcpsrepo_ior"
                             . " -d $domains_file");

#Test with multiple write threads and non blocking write.
$FooTest_1 = new PerlACE::Process ("FooTest3NoKey",
                            "-DCPSInfo file://$dcpsrepo_ior "
                            . "-t $num_threads_to_write -i $num_writes_per_thread "
                            . "-w $num_writers");

#Test write block waiting for available space with RELIABLE and 
#KEEP_ALL qos.
$FooTest_2 = new PerlACE::Process ("FooTest3NoKey",
                            "-DCPSInfo file://$dcpsrepo_ior "
                            . "-b 1 -t $num_threads_to_write -i $num_writes_per_thread "
                            . "-w $num_writers");

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
