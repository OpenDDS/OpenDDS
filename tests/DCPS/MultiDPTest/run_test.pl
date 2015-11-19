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

my @txtfiles = qw/publisher_finished.txt publisher_ready.txt
                  subscriber_finished.txt subscriber_ready.txt/;
unlink @txtfiles;

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

$test->setup_discovery();

$test->start_process('pub');
$test->start_process('sub');

my $result = $test->finish(300);
unlink @txtfiles;
exit $result;
