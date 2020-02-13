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

my $pub_ini = " pub_rtps.ini";
my $sub_ini = " sub_rtps.ini";

if ($test->flag('single')) {
    $sub_ini = $pub_ini;
}

$test->process("relay1", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-DCPSConfigFile relay1.ini -ApplicationDomain 42 -VerticalAddress 4444 -HorizontalAddress 127.0.0.1:11444 -UserData relay1" . $relay_security_opts);
$test->process("relay2", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-DCPSConfigFile relay2.ini -ApplicationDomain 42 -VerticalAddress 5444 -HorizontalAddress 127.0.0.1:11544 -UserData relay2" . $relay_security_opts);
$test->process("publisher", "publisher", "-ORBDebugLevel 1 -DCPSConfigFile". $pub_ini . $pub_sub_security_opts);
$test->process("subscriber", "subscriber", "-ORBDebugLevel 1 -DCPSConfigFile" . $sub_ini . $pub_sub_security_opts);

if ($test->flag('join')) {
    $test->start_process("relay2");
    sleep 3;
    $test->start_process("subscriber");
    sleep 3;
    $test->start_process("relay1");
    sleep 3;
    $test->start_process("publisher");
} else {
    $test->start_process("relay1");
    $test->start_process("relay2");
    sleep 3;
    $test->start_process("subscriber");
    $test->start_process("publisher");
}

$test->stop_process(180, "subscriber");
$test->stop_process(5, "publisher");

$test->kill_process(5, "relay1");
$test->kill_process(5, "relay2");

exit $test->finish();
