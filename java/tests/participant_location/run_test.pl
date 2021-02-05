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
my $debug = '0';
my $vmargs = "-ea";

my $pub_sub_ini = "rtps.ini";
my $opt = "";

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
    elsif ($i eq '-xcheck')
    {
      # disable -Xcheck:jni warnings
      $vmargs = "";
    }
    elsif ($i eq '-noice' || $i eq 'noice')
    {
      $pub_sub_ini = 'rtps_no_ice.ini';
      $opt = "-n";
    }
    elsif ($i eq '-security' || $i eq 'security')
    {
      $opt = "-s";
    }
}

my $debug_opt = ($debug eq '0') ? ''
    : "-ORBDebugLevel $debug -DCPSDebugLevel $debug";

my $test_opts = "$opt $debug_opt -ORBLogFile partLocTest.log -DCPSConfigFile $pub_sub_ini";

my $relay_security_opts = "-IdentityCA ../../../tests/security/certs/identity/identity_ca_cert.pem" .
  " -PermissionsCA ../../../tests/security/certs/permissions/permissions_ca_cert.pem" .
  " -IdentityCertificate ../../../tests/security/certs/identity/test_participant_01_cert.pem" .
  " -IdentityKey ../../../tests/security/certs/identity/test_participant_01_private_key.pem" .
  " -Governance governance_signed.p7s -Permissions permissions_relay_signed.p7s -DCPSSecurity 1";

PerlACE::add_lib_path ("$DDS_ROOT/java/tests/messenger/messenger_idl");

my $relay = new PerlDDS::TestFramework();
$relay->process("relay", "$ENV{DDS_ROOT}/bin/RtpsRelay", "-DCPSConfigFile relay.ini -ApplicationDomain 42 -VerticalAddress 4444 -HorizontalAddress 127.0.0.1:11444 $relay_security_opts");

my $psTest = new PerlDDS::Process_Java ("ParticipantLocationTest", $test_opts,
    ["$DDS_ROOT/java/tests/messenger/messenger_idl/messenger_idl_test.jar"], $vmargs);

use Data::Dumper;
print Dumper($psTest);

$relay->start_process("relay");
sleep(1);
$psTest->Spawn();

my $psTestResult = $psTest->WaitKill (30);

if ($psTestResult != 0) {
    print STDERR "ERROR: test publisher returned $psTestResult\n";
    $status = 1;
}

$relay->kill_process(5, "relay");

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
