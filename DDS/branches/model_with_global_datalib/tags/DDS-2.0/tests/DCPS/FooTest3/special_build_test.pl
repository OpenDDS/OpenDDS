eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

#!!!!!!! Must build with SPECIAL DEFINES - see README !!!!

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

#Clean the Foo.txt file which is used as a storage of the
#data written.

unlink "Foo.txt";

$status = 0;

# single writer with single instances test
$multiple_instance=0;
$num_threads_to_write=5;
$num_writes_per_thread=2;
$num_writers=1;
#max_samples_per_instance large enough.
$max_samples_per_instance=12345678;
$history_depth=1;
$blocking_write=0;

# single datawriter single instances blocking write test
if ($ARGV[0] eq 'b') {
  $blocking_write=1;
  $max_samples_per_instance=1;
}
# multiple instances test
elsif ($ARGV[0] eq 'mi') {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=1;
}
# multiple datawriters with multiple instances test
elsif ($ARGV[0] eq 'mw') {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=4;
}

# multiple instances with blocking write test
elsif ($ARGV[0] eq 'mib') {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=1;
  $blocking_write=1;
  $max_samples_per_instance=1;
}
# multiple datawriters with multiple instances and blocking write test
elsif ($ARGV[0] eq 'mwb') {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=4;
  $blocking_write=1;
  $max_samples_per_instance=1;
}
elsif ($ARGV[0] eq '') {
  #default test - single datawriter single instance and non-blocking test.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

$num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

$dcpsrepo_ior = "dcps_ir.ior";
# The pub_id_fname can not be a full path because the
# pub_id_fname will be part of the parameter of the -p option
# which will be parsed using ':' delimiter.
$pub_id_fname = "pub_id.txt";
$pub_port = PerlACE::random_port();
$sub_port = PerlACE::random_port();
$sub_id = 1;

unlink $dcpsrepo_ior;
unlink $pub_id_file;

# test multiple cases
$parameters = " -DCPSInfoRepo file://$dcpsrepo_ior -t $num_threads_to_write -w $num_writers"
              . " -m $multiple_instance -i $num_writes_per_thread "
              . " -n $max_samples_per_instance -d $history_depth -b $blocking_write";

$DCPSREPO = PerlDDS::create_process ("../../../../DDS/DCPSInfoRepo",
                               "-o $dcpsrepo_ior"
                               . " -ORBDebugLevel 1");

$FooTest3 = PerlDDS::create_process ("FooTest3", $parameters);

print STDERR "FooTest3 $parameters\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$FooTest3->Spawn ();

$result = $FooTest3->WaitKill (60);

if ($result != 0) {
    print STDERR "ERROR: FooTest3 returned $result \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

exit $status;
