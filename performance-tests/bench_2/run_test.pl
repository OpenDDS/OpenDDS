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

$test->{dcps_debug_level} = 0;
$test->{dcps_transport_debug_level} = 0;
$test->{add_transport_config} = 0;
$test->{add_orb_log_file} = 0;
$test->{add_pending_timeout} = 0;
my $tc_opts = "--wait-for-nodes 4 --override-create-time 10 example";
my $nc_opts = "one-shot --name test_nc_" . $$;
my $is_rtps_disc = 0;

my $flag_found = 1;
if ($test->flag('disco')) {
  $tc_opts .= " ci_disco";
  $is_rtps_disc = 1;
}
elsif ($test->flag('fan')) {
  $tc_opts .= " ci_fan --override-start-time 15";
  $is_rtps_disc = 1;
}
elsif ($test->flag('fan_frag')) {
  $tc_opts .= " ci_fan_frag --override-start-time 15";
  $is_rtps_disc = 1;
}
elsif ($test->flag('echo')) {
  $tc_opts .= " ci_echo --override-start-time 15";
  $is_rtps_disc = 1;
}
elsif ($test->flag('echo_frag')) {
  $tc_opts .= " ci_echo_frag --override-start-time 15";
  $is_rtps_disc = 1;
}
elsif ($test->flag('sm10')) {
  $tc_opts .= " showtime_mixed_10 --override-start-time 25";
  $is_rtps_disc = 1;
}
elsif ($test->flag('sm30')) {
  $tc_opts .= " showtime_mixed_30 --override-start-time 45";
  $is_rtps_disc = 1;
}
else {
  $flag_found = 0;
  $tc_opts .= " showtime_mixed_10 --override-start-time 25";
  $is_rtps_disc = 1;
}

$test->report_unused_flags(!$flag_found);

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log") unless $is_rtps_disc;

$test->process("node_controller", "node_controller/node_controller", $nc_opts);
$test->process("test_controller", "test_controller/test_controller", $tc_opts);

$test->start_process("node_controller");
$test->start_process("test_controller");

exit $test->finish(180);
