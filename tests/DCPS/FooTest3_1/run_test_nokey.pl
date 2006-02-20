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

$status = 0;

# The NO KEY type writer always have a single instance.
# We use the $multiple_instance=1 to let the test to
# give the different the key position value. This would
# verify the instance handle is only one handle with no key
# data type.
$multiple_instance=1;

$has_key = 0;
$num_threads_to_write=5;
$num_writes_per_thread=2;
$num_writers=1;
#Make max_samples_per_instance large enough.
$max_samples_per_instance= 12345678;
$history_depth=100;
$blocking_write=0;


# multiple datawriters test
if ($ARGV[0] eq 'mw') { 
  $num_threads_to_write=2; 
  $num_writers=2; 
}
# multiple datawriters test with blocking write
elsif ($ARGV[0] eq 'mwb') {  
  $num_writers=2; 
  $blocking_write=1;
}
#tbd: add test for message dropped due to the depth limit.
elsif ($ARGV[0] eq '') {
  # default test
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

$num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers + $num_writers;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");
$pubdriver_ior = PerlACE::LocalFile ("pubdriver.ior");
# The pub_id_fname can not be a full path because the 
# pub_id_fname will be part of the parameter of the -p option 
# which will be parsed using ':' delimiter.
$pub_id_fname = "pub_id.txt";
$pub_port = 5555;
$sub_port = 6666;
$sub_id = 1;

unlink $dcpsrepo_ior; 
unlink $pub_id_fname;
unlink $pubdriver_ior;

$DCPSREPO = new PerlACE::Process ("../../../dds/InfoRepo/DCPSInfoRepo",
                             "-o $dcpsrepo_ior"
                             . " -d $domains_file");

$publisher = new PerlACE::Process ("FooTest3NoKey_publisher", 
                                   "-p $pub_id_fname:localhost:$pub_port -s $sub_id:localhost:$sub_port "
                                   . " -DCPSInfoRepo file://$dcpsrepo_ior -t $num_threads_to_write -w $num_writers"
                                   . " -m $multiple_instance -i $num_writes_per_thread "
                                   . " -n $max_samples_per_instance -d $history_depth"
                                   . " -y $has_key -b $blocking_write -v $pubdriver_ior");
  
print $publisher->CommandLine(), "\n";

$subscriber = new PerlACE::Process ("FooTest3NoKey_subscriber", 
                                    "-p $pub_id_fname:localhost:$pub_port -s $sub_id:localhost:$sub_port "
                                    . "-n $num_writes -v file://$pubdriver_ior");

print $subscriber->CommandLine(), "\n";

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$subscriber->Spawn ();
$publisher->Spawn ();

$result = $publisher->WaitKill (60);

if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status = 1;
}


$result = $subscriber->WaitKill(60);

if ($result != 0) {
    print STDERR "ERROR: $subscriber returned $result  \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

exit $status;
