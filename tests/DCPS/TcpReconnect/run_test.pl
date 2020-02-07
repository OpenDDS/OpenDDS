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
my $dbg_lvl = '-ORBVerboseLogging 1 -DCPSBIT 0 -DCPSDebugLevel 1 -DCPSTransportDebugLevel 1 ';
my $pub_opts = "$dbg_lvl";
my $sub_opts = "$dbg_lvl";
my $repo_bit_opt = "-DCPSBIT 0";
my $is_rtps_disc = 0;
my $DCPSREPO;

my $thread_per_connection = "";
if ($test->flag('thread_per')) {
    $thread_per_connection = " -p ";
}

#
# stub parameters
#
my $stubPubSideHost  = "localhost";
my $stubPubSidePort = PerlACE::random_port();;
my $stubSubSideHost = "localhost";
my $stubSubSidePort = PerlACE::random_port();;
my $stubKillDelay = 4;
my $stubKillCount = 2;

#
# stub command and arguments.
#
# my $socatCmd = "/usr/bin/socat";
# my $socatArgs = " -d -d TCP-LISTEN:$socatPubSidePort,reuseaddr " .
#                 "TCP:stubSubSideHost:$stubSubSidePort";
my $subdir = $PerlACE::Process::ExeSubDir;
my $stubCmd = "";
if ($subdir) {
    $stubCmd = $subdir;
}
$stubCmd .= "stub";
my $stubArgs = " -stubp:$stubPubSideHost:$stubPubSidePort -stubs:$stubSubSideHost:$stubSubSidePort";
# $stubArgs .= " -stubv ";

#
# generate ini file for subscriber
#
my $resub = "[common]\n" .
    "DCPSGlobalTransportConfig=\$file\n\n" .
    "[transport/t1]\n" .
    "transport_type=tcp\n" .
    "datalink_release_delay=60000\n" .
    "conn_retry_backoff_multiplier=1.5\n" .
    "conn_retry_attempts=13\n" .
    "passive_reconnect_duration=60000\n" .
    "local_address=$stubSubSideHost:$stubSubSidePort\n" .
    "pub_address=$stubPubSideHost:$stubPubSidePort\n";

open(my $fh, '>', 'resub.ini');
print $fh $resub;
close $fh;

$pub_opts .= ' -DCPSConfigFile repub.ini';
$sub_opts .= ' -DCPSConfigFile resub.ini';
$sub_opts .= " -k $stubKillCount -d $stubKillDelay";

#clean up files left from previous run
my $stub_ready = 'stub_ready.txt';
unlink $stub_ready;

$pub_opts .= $thread_per_connection;
$pub_opts .= " -stubCmd $stubCmd $stubArgs -stub_ready_file $stub_ready -k $stubKillCount -d $stubKillDelay";

$test->setup_discovery("-NOBITS -ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log " .
                       "$repo_bit_opt");

$test->process("publisher", "publisher", $pub_opts);
$test->process("subscriber", "subscriber", $sub_opts);

$test->start_process("subscriber");
$test->start_process("publisher");

# ignore this issue that is already being tracked in redmine
$test->ignore_error("(Redmine Issue# 1446)");

# Ignore normal disconnect/reconnect messages
$test->ignore_error("TcpConnection::active_reconnect_i error Connection refused");

# start killing processes in 60 seconds
my $fin = $test->finish(60);

unlink $stub_ready;
unlink 'resub.ini';
exit $fin;
