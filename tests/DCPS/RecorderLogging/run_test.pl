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

my $dbg_lvl = '-ORBDebugLevel 1 -DCPSDebugLevel 4 -DCPSTransportDebugLevel 2 -DCPSPendingTimeout 3';
my $pub_opts = "$dbg_lvl -ORBLogFile pub.log";
my $recorder_opts = "$dbg_lvl -ORBLogFile recorder.log";
my $repo_bit_opt = "";
my $stack_based = 0;
my $is_rtps_disc = 0;
my $DCPSREPO;

unlink qw/DCPSInfoRepo.log pub.log recorder.log/;

my $arg = 0;
my $thread_per_connection = "";
if ($ARGV[0] eq 'thread_per' || $#ARGV > 0 && $ARGV[1] eq 'thread_per') {
    $thread_per_connection = " -p ";
    $arg = 1 if ($ARGV[0] eq 'thread_per');
}

if ($ARGV[$arg] eq 'udp') {
    $pub_opts .= " -DCPSConfigFile pub_udp.ini";
    $recorder_opts .= " -DCPSConfigFile recorder_udp.ini";
}
elsif ($ARGV[$arg] eq 'multicast') {
    $pub_opts .= " -DCPSConfigFile pub_multicast.ini";
    $recorder_opts .= " -DCPSConfigFile recorder_multicast.ini";
}
elsif ($ARGV[$arg] eq 'default_tcp') {
    $pub_opts .= " -t tcp";
    $recorder_opts .= " -t tcp";
}
elsif ($ARGV[$arg] eq 'default_udp') {
    $pub_opts .= " -t udp";
    $recorder_opts .= " -t udp";
}
elsif ($ARGV[$arg] eq 'default_multicast') {
    $pub_opts .= " -t multicast";
    $recorder_opts .= " -t multicast";
}
elsif ($ARGV[$arg] eq 'nobits') {
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= ' -DCPSConfigFile pub.ini -DCPSBit 0';
    $recorder_opts .= ' -DCPSConfigFile recorder.ini -DCPSBit 0';
}
elsif ($ARGV[$arg] eq 'ipv6') {
    $pub_opts .= " -DCPSConfigFile pub_ipv6.ini";
    $recorder_opts .= " -DCPSConfigFile recorder_ipv6.ini";
}
elsif ($ARGV[$arg] eq 'rtps') {
    $pub_opts .= " -DCPSConfigFile rtps.ini";
    $recorder_opts .= " -DCPSConfigFile rtps.ini";
}
elsif ($ARGV[$arg] eq 'rtps_disc') {
    $pub_opts .= " -DCPSConfigFile rtps_disc.ini";
    $recorder_opts .= " -DCPSConfigFile rtps_disc.ini";
    $is_rtps_disc = 1;
}
elsif ($ARGV[$arg] eq 'rtps_disc_tcp') {
    $pub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $recorder_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $is_rtps_disc = 1;
}
elsif ($ARGV[$arg] eq 'rtps_unicast') {
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= " -DCPSConfigFile rtps_uni.ini -DCPSBit 0";
    $recorder_opts .= " -DCPSConfigFile rtps_uni.ini -DCPSBit 0";
}
elsif ($ARGV[$arg] eq 'shmem') {
    $pub_opts .= " -DCPSConfigFile shmem.ini";
    $recorder_opts .= " -DCPSConfigFile shmem.ini";
}
elsif ($ARGV[$arg] eq 'all') {
    @original_ARGV = grep { $_ ne 'all' } @original_ARGV;
    my @tests = ('', qw/udp multicast default_tcp default_udp default_multicast
                        nobits shmem
                        rtps rtps_disc rtps_unicast rtps_disc_tcp/);
    push(@tests, 'ipv6') if new PerlACE::ConfigList->check_config('IPV6');
    for my $test (@tests) {
        $status += system($^X, $0, @original_ARGV, $test);
    }
    exit $status;
}
elsif ($ARGV[$arg] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}
else {
    $pub_opts .= ' -DCPSConfigFile pub.ini';
    $recorder_opts .= ' -DCPSConfigFile recorder.ini';
}

$pub_opts .= $thread_per_connection;

my $test = new PerlDDS::TestFramework();
$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt ") unless $is_rtps_disc;
$test->process("publisher",  "publisher", $pub_opts);
$test->process("recorder",      "recorder", $recorder_opts);
$test->start_process("publisher");
$test->start_process("recorder");

my $result = $test->finish(150);
if ($result != 0) {
    print STDERR "ERROR: test returned $result\n";
    $status = 1;
}

exit $status;
