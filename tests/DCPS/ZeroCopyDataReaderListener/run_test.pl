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

my $test = new PerlDDS::TestFramework();

if ($test->flag('all')) {
    @original_ARGV = grep { $_ ne 'all' } @original_ARGV;
    my @tests = ('', qw/udp multicast default_tcp default_udp default_multicast
                        nobits stack shmem
                        rtps rtps_disc rtps_unicast rtps_disc_tcp/);
    push(@tests, 'ipv6') if new PerlACE::ConfigList->check_config('IPV6');
    my $status = 0;
    for my $test (@tests) {
        $status += system($^X, $0, @original_ARGV, $test);
    }
    exit $status;
}

$test->{'dcps_debug_level'} = 4;
$test->{'dcps_transport_debug_level'} = 2;

my $dbg_lvl = '-ORBDebugLevel 1';
my $pending_timeout = '-DCPSPendingTimeout 2';

my $thread_per_connection = $test->flag('thread_per') ? '-p' : '';

my $default_tport = ($test->flag('default_tcp') ||
                     $test->flag('stack')) ? '-t tcp' :
    $test->flag('default_udp') ? '-t udp' :
    $test->flag('default_multicast') ? '-t multicast' : '';

$test->{'transport'} = ($PerlDDS::SafetyProfile ? 'rtps_disc' : 'tcp')
    if $default_tport eq '' && $test->{'transport'} eq '';

my $stack_based = $test->flag('stack') ? 1 : 0;

$test->process('sub', ($stack_based ? 'stack_' : '') . 'subscriber',
               "$dbg_lvl $default_tport $pending_timeout");
$test->process('pub', 'publisher',
               "$dbg_lvl $thread_per_connection $default_tport $pending_timeout");

$test->setup_discovery();
$test->start_process('pub');
$test->start_process('sub');

exit $test->finish(300);
