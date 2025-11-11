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
use PerlDDS::Response_Monitor;
use strict;

PerlDDS::add_lib_path('../../ConsolidatedMessengerIdl');
PerlDDS::add_lib_path('../../common');

my $status = 0;

my $test = new PerlDDS::TestFramework();

$test->{dcps_debug_level} = 1;
$test->{dcps_transport_debug_level} = 1;
# will manually set -DCPSConfigFile
$test->{add_transport_config} = 0;

my $relay_security_opts = "";
my $pub_sub_security_opts = "";
if ($test->flag('secure')) {
    $relay_security_opts = " -IdentityCA ../../../security/certs/identity/identity_ca_cert.pem -PermissionsCA ../../../security/certs/permissions/permissions_ca_cert.pem -IdentityCertificate ../../../security/certs/identity/test_participant_01_cert.pem -IdentityKey ../../../security/certs/identity/test_participant_01_private_key.pem -Governance governance_signed.p7s -Permissions permissions_relay_signed.p7s -DCPSSecurity 1";
    $pub_sub_security_opts = " -DCPSSecurity 1";
}

sub uses_one_relay {
    return $test->flag('partition_same_relay') || $test->flag('single') || $test->flag('draining') || $test->flag('deny_partitions');
}

my $pub_ini = " pub_rtps.ini";
my $sub_ini = " sub_rtps.ini";

if ($test->flag('single')) {
    $sub_ini = $pub_ini;
}
elsif ($test->flag('partition_same_relay')) {
    # Use with the 'secure' flag.
    # This puts the subscriber in a partition that doesn't match the publisher.
    # The config files are set up to allow the participants to exchange SPDP
    # without the help of the relay. The SEDP port, however, is not set up for
    # local (non-relayed) communication. Since the relay should not forward
    # when no partitions are in common, secure discovery cannot complete.
    $pub_ini = ' pub_same_relay.ini -b 1';
    $sub_ini = ' sub_same_relay.ini -b 1 -p OCJ';
}

sub get_relay_args {
    my $n = shift;
    my $port_digit = 3 + $n;
    return join(' ',
        "-Id relay${n}",
        "-UserData relay${n}",
        "-DCPSDebugLevel 1",
        "-DCPSSecurityDebugLevel 2",
        "-LogDiscovery 1",
        "-LogActivity 1",
        "-LogThreadStatus 1",
        "-LogRelayStatistics 3",
        "-DCPSConfigFile relay${n}.ini",
        "-ApplicationDomain 42",
        "-VerticalAddress ${port_digit}444",
        "-HorizontalAddress 127.0.0.1:11${port_digit}44",
        "-MetaDiscoveryAddress 127.0.0.1:808${n}",
        "-ORBVerboseLogging 1",
        "-SynchronousOutput 1"
    );
}

my $pub_extra_args = $test->flag('draining') ? ' -d' : ($test->flag('deny_partitions') ? ' -r' : '');
my $drain_args = '-Set RTPS_RELAY_DRAIN_INTERVAL=100 -Set RTPS_RELAY_DRAIN_STATE=Draining ' .
    '-Set RTPS_RELAY_ADMIT_STATE=NotAdmitting';
my $deny_partitions_args = '-Deny publisher -Set RTPS_RELAY_DENIED_PARTITIONS_TIMEOUT=10';
my $control_args = $test->flag('draining') ? $drain_args : ($test->flag('deny_partitions') ? $deny_partitions_args : '');

$test->process("monitor", "monitor", "-DCPSConfigFile monitor.ini");
$test->process("relay1", "$ENV{DDS_ROOT}/bin/RtpsRelay", get_relay_args(1) . $relay_security_opts);
$test->process("relay2", "$ENV{DDS_ROOT}/bin/RtpsRelay", get_relay_args(2) . $relay_security_opts) unless uses_one_relay();
$test->process("publisher", "publisher", "-ORBDebugLevel 1 -DCPSConfigFile". $pub_ini . $pub_sub_security_opts . $pub_extra_args);
$test->process("subscriber", "subscriber", "-ORBDebugLevel 1 -DCPSConfigFile" . $sub_ini . $pub_sub_security_opts);
$test->process("metachecker", "metachecker", "127.0.0.1:8081");
$test->process("control1", "$ENV{DDS_ROOT}/bin/RtpsRelayControl", "-RelayId relay1 -DCPSConfigFile relay1.ini $control_args");

# start a response monitor checking for
# > 500 ms of clock drift every second.
my $mon = new Response_Monitor();
sleep 2;

$test->start_process("monitor");

if ($test->flag('join')) {
    $test->start_process("relay2");
    sleep 3;
    $test->start_process("subscriber");
    sleep 3;
    $test->start_process("relay1");
    sleep 10;
    $test->start_process("publisher");
} else {
    $test->start_process("relay1");
    $test->start_process("relay2") unless uses_one_relay();
    sleep 10;
    $test->start_process("publisher");
    sleep 1;
    $test->start_process("subscriber") unless ($test->flag('draining') || $test->flag('deny_partitions'));
}
$test->start_process("metachecker");

if ($test->flag('draining') || $test->flag('deny_partitions')) {
    $test->wait_for('publisher', 'connected', max_wait => 5);
    $test->start_process("control1");
    $test->wait_kill("publisher", 30);
    $test->kill_process(5, "control1");
}

$test->stop_process(5, "metachecker");
$test->stop_process(20, "subscriber") unless $test->flag('draining');
$test->stop_process(5, "publisher") unless $test->flag('draining');

$test->kill_process(5, "relay1");
$test->kill_process(5, "relay2") unless uses_one_relay();
$test->kill_process(5, "monitor");

exit $test->finish();
