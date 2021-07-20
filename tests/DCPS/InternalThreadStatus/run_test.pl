eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $status = 0;

my $test = new PerlDDS::TestFramework();

$test->{dcps_debug_level} = 4; # Level where thread status info is logged.
$test->{dcps_transport_debug_level} = 1;
# will manually set -DCPSConfigFile
$test->{add_transport_config} = 0;

$test->process("publisher", "publisher", "-DCPSThreadStatusInterval 1 -ORBDebugLevel 1 -DCPSConfigFile rtps.ini");
$test->process("subscriber", "subscriber", "-DCPSThreadStatusInterval 1 -ORBDebugLevel 1 -DCPSConfigFile rtps.ini");

$test->start_process("subscriber");
sleep 1;
$test->start_process("publisher");

$test->stop_process(20, "subscriber");
$test->stop_process(20, "publisher");

exit $test->finish();
