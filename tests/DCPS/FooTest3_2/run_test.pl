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
use strict;

# Set the library path for the client to be able to load
# the FooType* library.
PerlDDS::add_lib_path('../FooType3Unbounded');

my $status=0;

# single writer with single instances test
my $multiple_instance=0;
my $num_instances=1;
my $num_threads_to_write=5;
my $num_writes_per_thread=2;
my $num_writers=1;
#Make max_samples_per_instance large enough.
my $max_samples_per_instance= 12345678;
my $history_depth=100;
my $blocking_write=0;
my $write_delay_msec=0;
my $receive_delay_msec=0;
my $check_data_dropped=0;
my $publisher_running_sec=30;
my $subscriber_running_sec=20;
my $repo_bit_conf = "-NOBITS";
my $app_bit_conf = "-DCPSBit 0";

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
  $write_delay_msec=0;
  $check_data_dropped=1;
  $receive_delay_msec=100;
  $publisher_running_sec=150;
  $subscriber_running_sec=120;
}
elsif ($ARGV[0] eq 'b') {
  # test of blocking write
  $blocking_write=1;
  $max_samples_per_instance=1;
  $num_threads_to_write=1;
  $num_writes_per_thread=1000;
  $write_delay_msec=0;
  $receive_delay_msec=100;
  $publisher_running_sec=150;
  $subscriber_running_sec=120;
}
elsif ($ARGV[0] eq '') {
  # default test
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

my $num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

my $dcpsrepo_ior="dcps.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO=PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "$repo_bit_conf -o $dcpsrepo_ior");

my $publisher=PerlDDS::create_process ("FooTest3_publisher",
                                       "$app_bit_conf"
                                       . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                       . " -t $num_threads_to_write"
                                       . " -w $num_writers"
                                       . " -m $multiple_instance"
                                       . " -i $num_writes_per_thread "
                                       . " -n $max_samples_per_instance"
                                       . " -d $history_depth"
                                       . " -l $write_delay_msec"
                                       . " -r $check_data_dropped "
                                       . " -b $blocking_write ");

print $publisher->CommandLine(), "\n";

my $subscriber=PerlDDS::create_process ("FooTest3_subscriber",
                                        "$app_bit_conf"
                                        . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                        . " -n $num_writes"
                                        . " -l $receive_delay_msec");

print $subscriber->CommandLine(), "\n";

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$subscriber->Spawn ();
$publisher->Spawn ();

my $result=$publisher->WaitKill ($publisher_running_sec);

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

my $ir=$DCPSREPO->TerminateWaitKill(5);

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
