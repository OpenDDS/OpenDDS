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

my $multiple_instance = 0;
my $num_samples_per_reader = 3;
my $num_readers = 1;
my $use_take = 0;

my $parameters = $PerlDDS::SafetyProfile ? '-DCPSConfigFile rtps_disc.ini'
    : '-DCPSConfigFile all.ini';

$parameters .= " -r $num_readers -t $use_take"
    . " -m $multiple_instance -i $num_samples_per_reader";

my $test = new PerlDDS::TestFramework();

if ($test->flag('udp')) {
  $parameters .= ' -us -up';
}
elsif ($test->flag('diff_trans')) {
  $parameters .= ' -up';
}
elsif ($test->flag('rtps')) {
  $parameters .= ' -rs -rp';
}
elsif ($test->flag('shmem')) {
  $parameters .= ' -ss -sp';
}

$test->enable_console_logging();
$test->process('main', 'main', $parameters);

$test->setup_discovery();
$test->start_process('main');

exit $test->finish(60);
