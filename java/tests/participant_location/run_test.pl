eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env qw(ACE_ROOT DDS_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Process_Java;
use strict;

my $status = 0;
my $debug = '1';
my $vmargs = "-ea";

my $security = "-s";
my $noice;
my $norelay;
my $ipv6;

my $pub_sub_ini = "rtps.ini";
my $opt;

my $use_relay = 1;

my $relay_security_opts = "-IdentityCA ../../../tests/security/certs/identity/identity_ca_cert.pem" .
  " -PermissionsCA ../../../tests/security/certs/permissions/permissions_ca_cert.pem" .
  " -IdentityCertificate ../../../tests/security/certs/identity/test_participant_01_cert.pem" .
  " -IdentityKey ../../../tests/security/certs/identity/test_participant_01_private_key.pem" .
  " -Governance governance_signed.p7s -Permissions permissions_relay_signed.p7s -DCPSSecurity 1";

foreach my $i (@ARGV) {
  if ($i eq '-debug') {
    $debug = '10';
  } elsif ($i eq 'nosecurity' || $i eq '-nosecurity') {
    $security = "";
    $relay_security_opts = "";
  } elsif ($i eq 'noice' || $i eq '-noice') {
    $noice = "-n";
  } elsif ($i eq 'norelay' || $i eq '-norelay') {
    $norelay = "-r";
    undef $use_relay;
  } elsif ($i eq 'ipv6' || $i eq '-ipv6') {
    $ipv6 = "-6";
  }
}

if ($noice && $ipv6) {
    $pub_sub_ini = 'rtps_no_ice_ipv6.ini';
} elsif ($noice) {
    $pub_sub_ini = 'rtps_no_ice.ini';
} elsif ($ipv6) {
    $pub_sub_ini = 'rtps_ipv6.ini';
}

$opt = "$security $noice $norelay $ipv6";

my $debug_opt = ($debug eq '0') ? ''
    : "-ORBDebugLevel $debug -DCPSDebugLevel $debug";

my $test_opts = "$opt $debug_opt -ORBLogFile partLocTest.log -DCPSConfigFile $pub_sub_ini";

PerlACE::add_lib_path ("$DDS_ROOT/java/tests/messenger/messenger_idl");

my $relay;

if ($use_relay) {
  $relay = new PerlDDS::TestFramework();
  if ($ipv6) {
    $relay->process("relay", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-DCPSConfigFile relay_ipv6.ini -ApplicationDomain 42 -VerticalAddress [::]:4444 -HorizontalAddress [::1]:11444 $relay_security_opts");
  } else {
    $relay->process("relay", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-DCPSConfigFile relay.ini -ApplicationDomain 42 -VerticalAddress 4444 -HorizontalAddress 127.0.0.1:11444 $relay_security_opts");
  }

  $relay->start_process("relay");
  sleep(1);
}

my $psTest = new PerlDDS::Process_Java ("ParticipantLocationTest", $test_opts,
    ["$DDS_ROOT/java/tests/messenger/messenger_idl/messenger_idl_test.jar"], $vmargs);

my $psTestResult = $psTest->SpawnWaitKill(30);

if ($psTestResult != 0) {
    print STDERR "ERROR: test publisher returned $psTestResult\n";
    $status = 1;
}

if ($use_relay) {
  $relay->kill_process(5, "relay");
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
