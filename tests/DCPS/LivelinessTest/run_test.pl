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

# single reader with single instances test
my $multiple_instance = 0;
my $num_samples_per_reader = 2;
my $num_unlively_periods = 3;
my $num_readers = 1;

my $use_take = 0;

my $test = new PerlDDS::TestFramework();
my $app_bit_conf = ($test->{'transport'} eq 'udp') ? '-DCPSBit 0' : '-r 1';

if ($test->flag('take')) {
  print "use_take !!!!!\n";
  $use_take = 1;
}

$test->setup_discovery();
$test->enable_console_logging();

my $common_parameters = $app_bit_conf
    . " -w $num_readers -m $multiple_instance"
    . " -l $num_unlively_periods -i $num_samples_per_reader";
$test->process('lt', 'LivelinessTest', $common_parameters);
$test->start_process('lt', "-t $use_take");

my $result = $test->finish(180);

exit $result;
