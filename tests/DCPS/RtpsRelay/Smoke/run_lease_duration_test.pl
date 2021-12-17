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

my $relay_security_opts = "";
my $pub_sub_security_opts = "";
if ($test->flag('secure')) {
    $relay_security_opts = " -IdentityCA ../../../security/certs/identity/identity_ca_cert.pem -PermissionsCA ../../../security/certs/permissions/permissions_ca_cert.pem -IdentityCertificate ../../../security/certs/identity/test_participant_01_cert.pem -IdentityKey ../../../security/certs/identity/test_participant_01_private_key.pem -Governance governance_signed.p7s -Permissions permissions_relay_signed.p7s -DCPSSecurity 1";
    $pub_sub_security_opts = " -DCPSSecurity 1";
}

my $pub_ini = " rtps_ld_60_sec.ini -e";
my $sub_ini = " rtps_ld_5_sec.ini";

if ($test->flag('reverse')) {
  $pub_ini = " rtps_ld_5_sec.ini";
  $sub_ini = " rtps_ld_60_sec.ini -e";
}

$test->process("monitor", "monitor", "-DCPSConfigFile monitor.ini");
$test->process("relay1a", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-Id relay1a -LogDiscovery 1 -LogActivity 1 -LogRelayStatistics 3 -DCPSConfigFile relay1.ini -ApplicationDomain 42 -VerticalAddress 4444 -HorizontalAddress 127.0.0.1:11444 -UserData relay1a" . $relay_security_opts);
$test->process("relay1b", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-Id relay1b -LogDiscovery 1 -LogActivity 1 -LogRelayStatistics 3 -DCPSConfigFile relay1.ini -ApplicationDomain 42 -VerticalAddress 4444 -HorizontalAddress 127.0.0.1:11444 -UserData relay1b" . $relay_security_opts);
$test->process("publisher", "publisher", "-l -ORBDebugLevel 1 -DCPSConfigFile". $pub_ini . $pub_sub_security_opts);
$test->process("subscriber", "subscriber", "-l -ORBDebugLevel 1 -DCPSConfigFile" . $sub_ini . $pub_sub_security_opts);

$test->start_process("monitor");

if ($test->flag('reverse')) {
  $test->start_process("relay1a");
  sleep 3;
  $test->start_process("publisher");
  $test->start_process("subscriber");
  sleep 10;
  $test->kill_process(1, "relay1a");
  sleep 8;
  $test->start_process("relay1b");
} else {
  $test->start_process("relay1a");
  sleep 3;
  $test->start_process("publisher");
  $test->start_process("subscriber");
  sleep 10;
  $test->kill_process(1, "relay1a");
  sleep 8;
  $test->start_process("relay1b");
}

sleep 30;

$test->kill_process(1, "relay1b");
$test->kill_process(5, "monitor");

exit $test->finish(15);
