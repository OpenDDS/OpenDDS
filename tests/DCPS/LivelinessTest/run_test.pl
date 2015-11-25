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

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

my $subscriber_completed = "subscriber_finished.txt";
my $subscriber_ready = "subscriber_ready.txt";
my $publisher_completed = "publisher_finished.txt";
my $publisher_ready = "publisher_ready.txt";

# single reader with single instances test
my $multiple_instance = 0;
my $num_samples_per_reader = 2;
my $num_unlively_periods = 3;
my $num_readers = 1;

my $use_take = 0;

my $test = new PerlDDS::TestFramework();
my $app_bit_conf = ($test->{'transport'} eq 'udp') ? '-DCPSBit 0' : '';

if ($test->flag('take')) {
  print "use_take !!!!!\n";
  $use_take = 1;
}

$test->setup_discovery();
$test->enable_console_logging();

my $common_parameters = $app_bit_conf
    . " -w $num_readers -m $multiple_instance"
    . " -l $num_unlively_periods -i $num_samples_per_reader";

$test->process('sub', 'subscriber', $common_parameters . " -t $use_take");
$test->process('pub', 'publisher', $common_parameters);

$test->add_temporary_file('sub', $subscriber_completed);
$test->add_temporary_file('sub', $subscriber_ready);
$test->add_temporary_file('pub', $publisher_completed);
$test->add_temporary_file('pub', $publisher_ready);

$test->start_process('sub', '-T');
$test->start_process('pub', '-T');

my $result = $test->finish(60);

exit $result;
