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

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
}

my $opts = "";
my $debug_opt = ($debug eq '0') ? ''
    : "-ORBDebugLevel $debug -DCPSDebugLevel $debug";

my $test_opts = "$opts $debug_opt -ORBLogFile test.log -DCPSConfigFile rtps.ini";

PerlACE::add_lib_path ("$DDS_ROOT/java/tests/messenger/messenger_idl");

my $TEST = new PerlDDS::Process_Java ("ParticipantLocationTest", $test_opts,
    ["$DDS_ROOT/java/tests/messenger/messenger_idl/messenger_idl_test.jar"]);

$TEST->Spawn ();
my $TestResult = $TEST->WaitKill (300);
if ($TestResult != 0) {
    print STDERR "ERROR: test returned $TestResult \n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
