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

PerlDDS::add_lib_path('../../ConsolidatedMessengerIdl');
PerlDDS::add_lib_path('../../common');

my $status = 0;

my $test = new PerlDDS::TestFramework();

$test->{dcps_debug_level} = 1;
$test->{dcps_transport_debug_level} = 1;
# will manually set -DCPSConfigFile
$test->{add_transport_config} = 0;

# Topology:
# publisher <====> relay1 <====> relay2 <====> subscriber
# Async discovery is enabled for relay1 but disabled for relay2.
# subscriber is permanently connected to relay2.
# relay1 builds up async discovery cache for publisher the first time it connects to relay1.
# When publisher reconnects to relay1, the constructed cache should be used.

# Require security
sub get_relay_args {
    my $n = shift;
    my $port_digit = 3 + $n;
    my $async_discovery_opts = $n == 1 ? "-CertificateIdPattern CN\\s*=\\s*(Tahoe).* -LogAsyncDiscovery 1" : "";
    return join(' ',
        "-Id relay${n}",
        $async_discovery_opts,
        "-LogDiscovery 1",
        "-LogActivity 1",
        "-LogRelayStatistics 3",
        "-DCPSConfigFile relay${n}.ini",
        "-ApplicationDomain 42",
        "-VerticalAddress ${port_digit}444",
        "-HorizontalAddress 127.0.0.1:11${port_digit}44",
        "-MetaDiscoveryAddress 127.0.0.1:808${n}",
        "-UserData relay${n}",
        "-IdentityCA ../../../security/certs/identity/identity_ca_cert.pem",
        "-PermissionsCA ../../../security/certs/permissions/permissions_ca_cert.pem",
        "-IdentityCertificate ../../../security/certs/identity/test_participant_01_cert.pem",
        "-IdentityKey ../../../security/certs/identity/test_participant_01_private_key.pem",
        "-Governance governance_signed.p7s",
        "-Permissions permissions_relay_signed.p7s",
        "-DCPSSecurity 1"
    );
}

my $pub_ini = " pub_rtps.ini";
my $sub_ini = " sub_rtps.ini";
if (!$test->flag('cross_relay')) {
    $sub_ini = $pub_ini;
}

$test->process("relay1", "$ENV{DDS_ROOT}/bin/RtpsRelay", get_relay_args(1));
$test->process("relay2", "$ENV{DDS_ROOT}/bin/RtpsRelay", get_relay_args(2)) if $test->flag('cross_relay');
$test->process("publisher", "publisher", "-ORBDebugLevel 1 -DCPSConfigFile". $pub_ini . " -DCPSSecurity 1");
$test->process("publisher2", "publisher", "-ORBDebugLevel 1 -DCPSConfigFile". $pub_ini . " -DCPSSecurity 1 -r");
$test->process("subscriber", "subscriber", "-ORBDebugLevel 1 -DCPSConfigFile" . $sub_ini . " -DCPSSecurity 1 -r");

# First, connect the first publisher with the subscriber
if ($test->flag('cross_relay')) {
    $test->start_process("relay2");
    sleep 3;
}

$test->start_process("subscriber");
sleep 3;
$test->start_process("relay1");
sleep 10;
$test->start_process("publisher");
sleep 3;
$test->stop_process(5, "publisher");

# Then, connect the second publisher with the subscriber.
# The partition cache constructed from the first publisher connection should be used.
$test->start_process("publisher2");
sleep 3;

$test->stop_process(5, "subscriber");
$test->stop_process(5, "publisher2");
$test->kill_process(5, "relay1");
if ($test->flag('cross_relay')) {
    $test->kill_process(5, "relay2");
}

exit $test->finish(15);
