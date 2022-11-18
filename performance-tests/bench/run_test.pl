eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use Env (TEST_RUN_PARAMS);
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
my $tc_opts = "--wait-for-nodes 7 example";
my $nc_opts = "one-shot --name test_nc_" . $$;
my $is_rtps_disc = 0;

if ($TEST_RUN_PARAMS ne "") {
  $tc_opts .= " $TEST_RUN_PARAMS";
}

if ($test->flag('--show-worker-logs')) {
  # This should be set for CI & autobuild runs to catch any error messages in worker DDS logs
  $tc_opts .= " --show-worker-logs";
}

if ($test->flag('--no-suffix')) {
  # Handy for 'joining' the scenario with an external worker (e.g. gdb, mutrace etc)
  $tc_opts .= " --override-bench-partition-suffix none";
}

if ($test->flag('--json')) {
  # This will result in an additional results file being written in json format
  $tc_opts .= " --json";
}

my $flag_found = 1;
if ($test->flag('ci-disco')) {
  $tc_opts .= " ci-disco";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-disco-long')) {
  $tc_opts .= " ci-disco-long";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-disco-repo')) {
  $tc_opts .= " ci-disco-repo";
  $is_rtps_disc = 1; # counter-intuitive, but we want bench to launch the repo, not the run script
}
elsif ($test->flag('ci-disco-relay')) {
  $tc_opts .= " ci-disco-relay";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-disco-relay-only')) {
  $tc_opts .= " ci-disco-relay-only";
  $is_rtps_disc = 1;
}
elsif ($test->flag('script')) {
  $tc_opts .= " script";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-fan')) {
  $tc_opts .= " ci-fan";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-fan-ws')) {
  $tc_opts .= " ci-fan-ws";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-fan-frag')) {
  $tc_opts .= " ci-fan-frag";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-fan-frag-ws')) {
  $tc_opts .= " ci-fan-frag-ws";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-echo')) {
  $tc_opts .= " ci-echo";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-echo-frag')) {
  $tc_opts .= " ci-echo-frag";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-force-worker-segfault')) {
  $tc_opts .= " ci-force-worker-segfault";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-force-worker-assert')) {
  $tc_opts .= " ci-force-worker-assert";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-force-worker-deadlock')) {
  $tc_opts .= " ci-force-worker-deadlock";
  $is_rtps_disc = 1;
}
elsif ($test->flag('ci-mixed')) {
  $tc_opts .= " ci-mixed";
  $is_rtps_disc = 1;
}
elsif ($test->flag('sm10')) {
  $tc_opts .= " showtime-mixed-10";
  $is_rtps_disc = 1;
}
elsif ($test->flag('sm20')) {
  $tc_opts .= " showtime-mixed-20";
  $is_rtps_disc = 1;
}
elsif ($test->flag('sm30')) {
  $tc_opts .= " showtime-mixed-30";
  $is_rtps_disc = 1;
}
elsif ($test->flag('simple-tags')) {
  $tc_opts .= " simple-tags --tag continuous --tag control --tag processed --tag unknown";
  $is_rtps_disc = 1;
  $test->process("node_controller2", "node_controller/node_controller", $nc_opts . '_2');
}
elsif ($test->flag('script')) {
  $tc_opts .= " script";
  $is_rtps_disc = 1;
}
elsif ($test->flag('valgrind')) {
  $tc_opts .= " valgrind";
  $is_rtps_disc = 1;
}
elsif ($test->flag('udp-latency')) {
  $tc_opts .= " udp-latency";
  $is_rtps_disc = 1;
}
elsif ($test->flag('tcp-latency')) {
  $tc_opts .= " tcp-latency";
  $is_rtps_disc = 1;
}
else {
  $flag_found = 0;
  $tc_opts .= " ci-mixed";
  $is_rtps_disc = 1;
}

$test->report_unused_flags(!$flag_found);

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log") unless $is_rtps_disc;

$test->process("node_controller", "node_controller/node_controller", $nc_opts);
$test->process("test_controller", "test_controller/test_controller", $tc_opts);

if ($test->flag('simple-tags')) {
  $test->start_process("node_controller2");
}
$test->start_process("node_controller");
$test->start_process("test_controller");

exit $test->finish(180);
