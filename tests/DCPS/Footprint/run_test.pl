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
my $pub_exe = "tcppublisher";
my $sub_exe = "tcpsubscriber";

my $thread_per_connection = "";
if ($test->flag('thread_per')) {
    $thread_per_connection = " -p ";
}

my $flag_found = 1;
if ($test->flag('rtps')) {
    $pub_exe = "rtpspublisher";
    $sub_exe = "rtpssubscriber";
    $pub_opts .= " -DCPSConfigFile rtps.ini";
    $sub_opts .= " -DCPSConfigFile rtps.ini";
    $test->{discovery} = "rtps";
}
elsif ($test->flag('default_tcp')) {
    $pub_opts .= " -t tcp";
    $sub_opts .= " -t tcp";
}
elsif ($test->flag('all')) {
    @original_ARGV = grep { $_ ne 'all' } @original_ARGV;
    my @tests = ('', qw/default_tcp rtps/);
    for my $test (@tests) {
        $status += system($^X, $0, @original_ARGV, $test);
    }
    exit $status;
}
else {
    $flag_found = 0;
    $pub_opts .= ' -DCPSConfigFile pub.ini';
    $sub_opts .= ' -DCPSConfigFile sub.ini';
}

$test->report_unused_flags(!$flag_found);

$pub_opts .= $thread_per_connection;

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log " .
                       "$repo_bit_opt");

$test->process("publisher", $pub_exe, $pub_opts);
$test->process("subscriber", $sub_exe, $sub_opts);

$test->start_process("publisher");
$test->start_process("subscriber");

# ignore this issue that is already being tracked in redmine
$test->ignore_error("(Redmine Issue# 1446)");
# start killing processes in 300 seconds
exit $test->finish(300);
