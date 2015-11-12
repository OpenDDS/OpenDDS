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

PerlDDS::add_lib_path('../../Utils');

# process control parameters
my $keyed_data = 1;
my $multiple_instance = 0;
my $num_instances = 1;
my $num_writers = 1;
my $debug_level = 0;

# const process control parameters
my $num_threads_to_write = 5;
my $num_writes_per_thread = 2;
my $max_samples_per_instance = 12345678;
my $history_depth = 100;
my $write_delay_msec = 0;
my $receive_delay_msec = 0;
my $publisher_running_sec = 60;
my $subscriber_running_sec = 30;
my $repo_bit_conf = '-NOBITS';
my $app_bit_conf = '-DCPSBit 0';

my $config_file = "";

if ((new PerlACE::ConfigList)->check_config('OPENDDS_SAFETY_PROFILE')) {
  $config_file = "-DCPSConfigFile rtps.ini";
}

my $test = new PerlDDS::TestFramework();

if ($test->flag('keyed')) {
  $keyed_data = 1;
}
elsif ($test->flag('nokey')) {
  $keyed_data = 0;
}

if ($test->flag('single_instance')) {
  $multiple_instance = 0;
}
elsif ($test->flag('multiple_instance')) {
  $multiple_instance = 1;
}

if ($test->flag('single_datawriter')) {
  $num_writers = 1;
  $num_instances = $num_threads_to_write;
}
elsif ($test->flag('multiple_datawriter')) {
  $num_writers = 4;
  $num_instances = $num_threads_to_write * $num_writers;
}

if ($test->flag('debug')) {
  $debug_level = 4;
}

my $num_writes = $num_threads_to_write * $num_writes_per_thread * $num_writers;

$test->setup_discovery($repo_bit_conf);
$test->enable_console_logging();

$test->process('pub', 'publisher', $app_bit_conf
               . " -DCPSDebugLevel $debug_level"
               . " -keyed_data $keyed_data"
               . " -num_writers $num_writers"
               . " -history_depth $history_depth"
               . " -num_threads_to_write $num_threads_to_write"
               . " -multiple_instances $multiple_instance"
               . " -num_writes_per_thread $num_writes_per_thread "
               . " -max_samples_per_instance $max_samples_per_instance"
               . " -write_delay_msec $write_delay_msec"
               . " $config_file"
    );

$test->process('sub', 'subscriber', $app_bit_conf
               . " -DCPSDebugLevel $debug_level"
               . " -keyed_data $keyed_data"
               . " -num_writes $num_writes"
               . " -receive_delay_msec $receive_delay_msec"
               . " $config_file"
    );

$test->start_process('pub');
$test->start_process('sub');

exit $test->finish(($publisher_running_sec > $subscriber_running_sec)
                   ? $publisher_running_sec : $subscriber_running_sec);
