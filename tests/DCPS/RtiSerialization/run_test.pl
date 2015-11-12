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

my $status = 0;

my $test = new PerlDDS::TestFramework();

$test->{dcps_debug_level} = 4;
$test->{dcps_transport_debug_level} = 2;
# will manually set -DCPSConfigFile
$test->{add_transport_config} = 0;
my $dbg_lvl = '-ORBDebugLevel 1';
my $pub_opts = "$dbg_lvl";
my $sub_opts = "$dbg_lvl";
my $repo_bit_opt = "";
my $stack_based = 0;
my $DCPSREPO;

my $thread_per_connection = "";
if ($test->flag('thread_per')) {
    $thread_per_connection = " -p ";
}

my $flag_found = 1;
if ($test->flag('rtps')) {
    $pub_opts .= " -DCPSConfigFile rtps.ini";
    $sub_opts .= " -DCPSConfigFile rtps.ini";
}
elsif ($test->flag('rtps_disc_tcp')) {
    $pub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $sub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
}
elsif ($test->flag('rtps_unicast')) {
    $test->{nobits} = 1;
    $pub_opts .= " -DCPSConfigFile rtps_uni.ini";
    $sub_opts .= " -DCPSConfigFile rtps_uni.ini";
}
else {
    $flag_found = 0;
    $pub_opts .= ' -DCPSConfigFile tcp.ini';
    $sub_opts .= ' -DCPSConfigFile tcp.ini';
}

$pub_opts .= " -DCPSRTISerialization";
$sub_opts .= " -DCPSRTISerialization";

$test->report_unused_flags(!$flag_found);

$pub_opts .= $thread_per_connection;

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log " .
                       "$repo_bit_opt");

$test->process("publisher", "publisher", $pub_opts);
my $sub_exe = ($stack_based ? 'stack_' : '') . "subscriber";
$test->process("subscriber", $sub_exe, $sub_opts);

$test->start_process("subscriber");
$test->start_process("publisher");

# ignore this issue that is already being tracked in redmine
$test->ignore_error("(Redmine Issue# 1446)");
# start killing processes in 120 seconds
exit $test->finish(120);
