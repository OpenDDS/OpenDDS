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

my $num_instances_per_writer = 1;
my $num_samples_per_instance = 10;
my $args = "-m $num_instances_per_writer -i $num_samples_per_instance";
$args .= " -DCPSDebugLevel 3";

if ((new PerlACE::ConfigList)->check_config('OPENDDS_SAFETY_PROFILE')) {
  $args .= " -DCPSConfigFile memory_pool.ini"
}

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();
$test->process('sub', 'subscriber', $args);
$test->process('pub', 'publisher', $args);

$test->add_temporary_file('sub', 'subscriber_finished.txt');
$test->add_temporary_file('sub', 'subscriber_ready.txt');
$test->add_temporary_file('pub', 'publisher_finished.txt');
$test->add_temporary_file('pub', 'publisher_ready.txt');

$test->setup_discovery();

$test->start_process('pub', '-o');
$test->start_process('sub', '-o');

exit $test->finish(300);
