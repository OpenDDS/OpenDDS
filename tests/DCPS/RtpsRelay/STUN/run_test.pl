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

$test->{dcps_debug_level} = 1;
$test->{dcps_transport_debug_level} = 1;
# will manually set -DCPSConfigFile
$test->{add_transport_config} = 0;

if ($test->flag('ipv6')) {
    $test->process("server", "$DDS_ROOT/bin/RtpsRelay", "-Id relay -DCPSConfigFile relay_ipv6.ini -ApplicationDomain 42 -VerticalAddress [::1]:4444 -HorizontalAddress [::1]:11444");
    $test->process("client", "StunClient", "-ipv6 1");
} else {
    $test->process("server", "$DDS_ROOT/bin/RtpsRelay", "-Id relay -DCPSConfigFile relay.ini -ApplicationDomain 42 -VerticalAddress 127.0.0.1:4444 -HorizontalAddress 127.0.0.1:11444");
    $test->process("client", "StunClient");
}

$test->start_process("server");
sleep 3;
$test->start_process("client");

$test->stop_process(5, "client");
$test->kill_process(5, "server");

exit $test->finish();
