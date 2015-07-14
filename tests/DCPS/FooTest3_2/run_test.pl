eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

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

my $test = new PerlDDS::TestFramework();

my $multiple_instance=0;
my $num_threads_to_write=1;
my $num_writes_per_thread=100;
my $num_writers=1;
#Make max_samples_per_instance large enough.
my $max_samples_per_instance=12345678;
my $history_depth=100;
my $blocking_write=0;
my $write_delay_msec=0;
my $receive_delay_msec=100;
my $check_data_dropped=0;
my $publisher_running_sec=150;
$test->{nobits} = 1;
$test->{add_pending_timeout} = 0;
$test->enable_console_logging();

# multiple instances test
if ($test->flag('mi')) {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=1;
}
# multiple datawriters with multiple instances test
elsif ($test->flag('mw')) {
  $multiple_instance=1;
  $num_threads_to_write=5;
  $num_writes_per_thread=2;
  $num_writers=4;
}
#tbd: add test for message dropped due to the depth limit.
elsif ($test->flag('bp_remove')) {
  # test of non-blocking write under backpressure
  $history_depth=1;
  $check_data_dropped=1;
}
elsif ($test->flag('b')) {
  # test of blocking write
  $blocking_write=1;
  #Make write block potentially every 100 sends
  $max_samples_per_instance=25;
}

$test->report_unused_flags(1);

my $num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

$test->setup_discovery();

my $pub_parameters = " -t $num_threads_to_write"
                     . " -w $num_writers"
                     . " -m $multiple_instance"
                     . " -i $num_writes_per_thread "
                     . " -n $max_samples_per_instance"
                     . " -d $history_depth"
                     . " -l $write_delay_msec"
                     . " -r $check_data_dropped "
                     . " -b $blocking_write ";
my $sub_parameters = " -n $num_writes"
                     . " -l $receive_delay_msec";

$test->process("subscriber", "FooTest3_subscriber", $sub_parameters);
$test->process("publisher", "FooTest3_publisher", $pub_parameters);

$test->start_process("publisher");
$test->start_process("subscriber");

my $status = $test->finish($publisher_running_sec, "publisher");

exit $status;
