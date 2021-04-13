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

PerlDDS::add_lib_path('./lib');

my $status = 0;

my $test = new PerlDDS::TestFramework();

my $config_file = @ARGV[0];

$test->{dcps_debug_level} = 0;
$test->{dcps_transport_debug_level} = 0;
$test->{add_transport_config} = 0;
$test->{add_orb_log_file} = 0;
$test->{add_pending_timeout} = 0;

$test->process("worker", "worker", $config_file);

$test->start_process("worker");

exit $test->finish(180);
