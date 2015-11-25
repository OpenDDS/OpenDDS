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

PerlDDS::add_lib_path('../FooType5');

# single reader with single instances test

my $num_writers = 1;
my $num_instances_per_writer = 1;
my $num_samples_per_instance = 100;
my $use_udp = 0;
my $use_multicast = 0;
my $use_rtps_transport = 0;
my $use_shmem = 0;
my $num_readers = 1;
my $max_samples_per_instance = 1000;
my $sequence_length = 10;
my $no_key = 0;
my $write_interval_ms = 0;
my $writer_blocking_ms = 0;
my $read_interval_ms = 0;
my $mixed_trans = 0;

my $test = new PerlDDS::TestFramework();

if ($test->flag('nokey')) {
  $no_key = 1;
}

if ($test->flag('udp')) {
  $use_udp = 1;
  $write_interval_ms = 50;
}
elsif ($test->flag('multicast')) {
  $use_multicast = 1;
  $write_interval_ms = 50;
}
elsif ($test->flag('rtps') || $PerlDDS::SafetyProfile) {
  $use_rtps_transport = 1;
  $write_interval_ms = 50;
}
elsif ($test->flag('shmem')) {
  $use_shmem = 1;
  $write_interval_ms = 50;
}

# multiple instances, single datawriter, single datareader
if ($test->flag('mi')) {
  $num_instances_per_writer = 2;
}
elsif ($test->flag('mwmr')) {
  $num_samples_per_instance = 50;
  $num_writers = 2;
  $num_readers = 2;
}
elsif ($test->flag('mr')) {
  $num_samples_per_instance = 50;
  $num_writers = 1;
  $num_readers = 2;
}
elsif ($test->flag('mwmr_long_seq')) {
  $sequence_length = 256;
  $num_samples_per_instance = 50;
  $num_writers = 2;
  $num_readers = 2;
}
elsif ($test->flag('blocking')) {
  $writer_blocking_ms = 1000000; # 1000 seconds
  $num_instances_per_writer = 5;
  $num_samples_per_instance = 20;
  $max_samples_per_instance = 1;
  $read_interval_ms = 1000;
}
elsif ($test->flag('blocking_timeout')) {
  $writer_blocking_ms = 5; # milliseconds
  $num_instances_per_writer = 5;
  $num_samples_per_instance = 50;
  $max_samples_per_instance = 1;
  $sequence_length = 1000;
  $read_interval_ms = 1000;
}
elsif ($test->flag('mixed_trans')) {
  $num_samples_per_instance = 50;
  $num_writers = 2;
  $num_readers = 2;
  $mixed_trans = 1;
}

my $subscriber_completed = 'subscriber_finished.txt';
my $subscriber_ready = 'subscriber_ready.txt';
my $publisher_completed = 'publisher_finished.txt';
my $publisher_ready = 'publisher_ready.txt';

$test->enable_console_logging();

$test->report_unused_flags(1);

my $orig_ACE_LOG_TIMESTAMP = $ENV{ACE_LOG_TIMESTAMP};
$ENV{ACE_LOG_TIMESTAMP} = "TIME";
sub cleanup
{
  $ENV{ACE_LOG_TIMESTAMP} = $orig_ACE_LOG_TIMESTAMP;
}

$test->setup_discovery();

$test->{dcps_debug_level} = 1;

my $cfg = $PerlDDS::SafetyProfile ? 'rtps.ini' : 'all.ini';

my $sub_parameters = "-DCPSConfigFile $cfg -u $use_udp -c $use_multicast"
    . " -p $use_rtps_transport -s $use_shmem -r $num_readers "
    . " -m $num_instances_per_writer -i $num_samples_per_instance"
    . " -w $num_writers -z $sequence_length"
    . " -k $no_key -y $read_interval_ms -f $mixed_trans";

my $pub_parameters = "-DCPSConfigFile $cfg -u $use_udp -c $use_multicast "
    . " -p $use_rtps_transport -s $use_shmem -r $num_readers -w $num_writers "
    . " -m $num_instances_per_writer -i $num_samples_per_instance "
    . " -n $max_samples_per_instance -z $sequence_length"
    . " -k $no_key -y $write_interval_ms -b $writer_blocking_ms"
    . " -f $mixed_trans";

$test->process("subscriber", "subscriber", $sub_parameters);
$test->process("publisher", "publisher", $pub_parameters);

$test->add_temporary_file("subscriber", $subscriber_completed);
$test->add_temporary_file("subscriber", $subscriber_ready);
$test->add_temporary_file("publisher", $publisher_completed);
$test->add_temporary_file("publisher", $publisher_ready);

$test->start_process("publisher", "-o");
$test->start_process("subscriber", "-o");

my $status = $test->finish(300, "publisher");

cleanup();
exit $status;
