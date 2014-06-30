eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $dbg_lvl = '-ORBDebugLevel 1 -DCPSDebugLevel 4 -DCPSTransportDebugLevel 2';
my $pub_opts = "$dbg_lvl -ORBLogFile pub.log";
my $sub_opts = "$dbg_lvl -ORBLogFile sub.log";
my $repo_bit_opt = "";
my $stack_based = 0;
my $is_rtps_disc = 0;
my $DCPSREPO;

unlink qw/DCPSInfoRepo.log pub.log sub.log/;

my $test = new PerlDDS::TestFramework();

my $thread_per_connection = "";
if ($test->flag('thread_per')) {
    $thread_per_connection = " -p ";
}

my $flag_found = 1;
if ($test->flag('udp')) {
    $pub_opts .= " -DCPSConfigFile pub_udp.ini";
    $sub_opts .= " -DCPSConfigFile sub_udp.ini";
}
elsif ($test->flag('multicast')) {
    $pub_opts .= " -DCPSConfigFile pub_multicast.ini";
    $sub_opts .= " -DCPSConfigFile sub_multicast.ini";
}
elsif ($test->flag('default_tcp')) {
    $pub_opts .= " -t tcp";
    $sub_opts .= " -t tcp";
}
elsif ($test->flag('default_udp')) {
    $pub_opts .= " -t udp";
    $sub_opts .= " -t udp";
}
elsif ($test->flag('default_multicast')) {
    $pub_opts .= " -t multicast";
    $sub_opts .= " -t multicast";
}
elsif ($test->flag('nobits')) {
    # nobits handled by TestFramework
    $pub_opts .= ' -DCPSConfigFile pub.ini';
    $sub_opts .= ' -DCPSConfigFile sub.ini';
}
elsif ($test->flag('ipv6')) {
    $pub_opts .= " -DCPSConfigFile pub_ipv6.ini";
    $sub_opts .= " -DCPSConfigFile sub_ipv6.ini";
}
elsif ($test->flag('stack')) {
    $pub_opts .= " -t tcp";
    $sub_opts .= " -t tcp";
    $stack_based = 1;
}
elsif ($test->flag('rtps')) {
    $pub_opts .= " -DCPSConfigFile rtps.ini";
    $sub_opts .= " -DCPSConfigFile rtps.ini";
}
elsif ($test->flag('rtps_disc')) {
    $pub_opts .= " -DCPSConfigFile rtps_disc.ini";
    $sub_opts .= " -DCPSConfigFile rtps_disc.ini";
    $is_rtps_disc = 1;
}
elsif ($test->flag('rtps_disc_tcp')) {
    $pub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $sub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $is_rtps_disc = 1;
}
elsif ($test->flag('rtps_unicast')) {
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= " -DCPSConfigFile rtps_uni.ini -DCPSBit 0";
    $sub_opts .= " -DCPSConfigFile rtps_uni.ini -DCPSBit 0";
}
elsif ($test->flag('shmem')) {
    $pub_opts .= " -DCPSConfigFile shmem.ini";
    $sub_opts .= " -DCPSConfigFile shmem.ini";
}
elsif ($test->flag('all')) {
    @original_ARGV = grep { $_ ne 'all' } @original_ARGV;
    my @tests = ('', qw/udp multicast default_tcp default_udp default_multicast
                        nobits stack shmem
                        rtps rtps_disc rtps_unicast rtps_disc_tcp/);
    push(@tests, 'ipv6') if new PerlACE::ConfigList->check_config('IPV6');
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
$pub_opts .= " -DCPSPendingTimeout 1 ";
$sub_opts .= " -DCPSPendingTimeout 1 ";

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log " .
                       "$repo_bit_opt");

my $sub_exe = ($stack_based ? 'stack_' : '') . "subscriber";
$test->process("subscriber", $sub_exe, $sub_opts);
$test->process("publisher", "publisher", $pub_opts);

$test->start_process("subscriber");
$test->start_process("publisher");

# start killing processes in 300 seconds
exit $test->finish(300);
