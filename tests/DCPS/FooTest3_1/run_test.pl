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

# Set the library path for the client to be able to load
# the FooType* library.
PerlDDS::add_lib_path('../FooType3');

$status=0;

# single writer with single instances test
$multiple_instance=0;
$num_instances=1;
$num_threads_to_write=5;
$num_writes_per_thread=2;
$num_writers=1;
#Make max_samples_per_instance large enough.
$max_samples_per_instance= 12345678;
$history_depth=100;
$blocking_write=0;
$write_dalay_msec=0;
$receive_dalay_msec=0;
$check_data_dropped=0;
$publisher_running_sec=60;
$subscriber_running_sec=30;
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-DCPSBit 0";
$sub_ready_file = "sub_ready.txt";

# multiple instances test
if ($ARGV[0] eq 'mi') {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=1;
  $num_instances=$num_threads_to_write;
}
# multiple datawriters with multiple instances test
elsif ($ARGV[0] eq 'mw') {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=4;
  $num_instances=$num_threads_to_write * $num_writers;
}
#tbd: add test for message dropped due to the depth limit.
elsif ($ARGV[0] eq 'bp_remove') {
  # test of non-blocking write under backpressure
  $history_depth=1;
  $num_threads_to_write=1;
  $num_writes_per_thread=1000;
  $write_dalay_msec=0;
  $check_data_dropped=1;
  $receive_dalay_msec=100;
  $publisher_running_sec=120;
}
elsif ($ARGV[0] eq 'b') {
  # test of blocking write
  $blocking_write=1;
  $max_samples_per_instance=1;
  $num_threads_to_write=1;
  $num_writes_per_thread=1000;
  $write_dalay_msec=0;
  $receive_dalay_msec=100;
  $publisher_running_sec=120;
  $subscriber_running_sec=120;
}
elsif ($ARGV[0] eq '') {
  # default test
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

$num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers + $num_instances;

$dcpsrepo_ior="dcps_ir.ior";
$pubdriver_ior="pubdriver.ior";
# The pub_id_fname can not be a full path because the
# pub_id_fname will be part of the parameter of the -p option
# which will be parsed using ':' delimiter.
$pub_id_fname="pub_id.txt";
$pub_port=PerlACE::random_port();
$sub_port=PerlACE::random_port();
$sub_id=1;

unlink $dcpsrepo_ior;
unlink $pub_id_fname;
unlink $pubdriver_ior;
unlink $sub_ready_file;

$svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : " -ORBSvcConf ../../tcp.conf ";

$DCPSREPO=PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "$repo_bit_conf -o $dcpsrepo_ior"
                                  . " $svc_config");

$publisher=PerlDDS::create_process ("FooTest3_publisher"
                                   , "$svc_config"
                                   . "$app_bit_conf -p $pub_id_fname:localhost:$pub_port -s $sub_id:localhost:$sub_port "
                                   . " -DCPSInfoRepo file://$dcpsrepo_ior -t $num_threads_to_write -w $num_writers"
                                   . " -m $multiple_instance -i $num_writes_per_thread "
                                   . " -n $max_samples_per_instance -d $history_depth"
                                   . " -v $pubdriver_ior -l $write_dalay_msec -r $check_data_dropped "
                                   . " -b $blocking_write -f $sub_ready_file");

$subscriber=PerlDDS::create_process ("FooTest3_subscriber",
                                    , "$svc_config"
                                    . "$app_bit_conf -p $pub_id_fname:localhost:$pub_port -s $sub_id:localhost:$sub_port "
                                    . " -n $num_writes -v file://$pubdriver_ior -l $receive_dalay_msec -f $sub_ready_file");


print $DCPSREPO->CommandLine(), "\n";
$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


print $subscriber->CommandLine(), "\n";
$subscriber->Spawn ();

print $publisher->CommandLine(), "\n";
$publisher->Spawn ();

$result=$publisher->WaitKill ($publisher_running_sec);

if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status=1;
}

if ($check_data_dropped == 0) {
    $result=$subscriber->WaitKill($subscriber_running_sec);

    if ($result != 0) {
        print STDERR "ERROR: $subscriber returned $result  \n";
        $status=1;
    }
}
else {
    $result=$subscriber->TerminateWaitKill(5);

    if ($result != 0) {
        print STDERR "ERROR: subscriber returned $result\n";
        $status=1;
    }
}

$ir=$DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status=1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
