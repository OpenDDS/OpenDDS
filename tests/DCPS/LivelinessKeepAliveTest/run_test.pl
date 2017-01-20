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

PerlDDS::add_lib_path('../common');

my $subscriber_completed = "subscriber_finished.txt";
my $subscriber_ready = "subscriber_ready.txt";
my $publisher_completed = "publisher_finished.txt";
my $publisher_ready = "publisher_ready.txt";

my $test = new PerlDDS::TestFramework();
my $app_bit_conf = ($test->{'transport'} eq 'udp') ? '-DCPSBit 0' : '';

$test->setup_discovery();
$test->enable_console_logging();

my $common_parameters = "";

$test->process('sub', 'subscriber', $common_parameters);
$test->process('pub', 'publisher', $common_parameters . "SATELLITE_ONE");

$test->add_temporary_file('sub', $subscriber_completed);
$test->add_temporary_file('sub', $subscriber_ready);
$test->add_temporary_file('pub', $publisher_completed);
$test->add_temporary_file('pub', $publisher_ready);

$test->start_process('sub');
$test->start_process('pub');

my $result = $test->finish(60);

exit $result;
