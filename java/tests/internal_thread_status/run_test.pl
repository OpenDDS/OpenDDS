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
my $vmargs;

my $pub_sub_ini = "rtps.ini";
my $opt = "";

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
    elsif ($i eq '-noXcheck')
    {
      # disable -Xcheck:jni warnings
      $vmargs = "-ea";
    }
}

my $debug_opt = ($debug eq '0') ? ''
    : "-ORBDebugLevel $debug -DCPSDebugLevel $debug";

my $pub_test_opts = "$opt $debug_opt -ORBLogFile pubtest.log -DCPSThreadStatusInterval 1 -DCPSConfigFile $pub_sub_ini";
my $sub_test_opts = "$opt $debug_opt -ORBLogFile subtest.log -DCPSThreadStatusInterval 1 -DCPSConfigFile $pub_sub_ini";

PerlACE::add_lib_path ("$DDS_ROOT/java/tests/messenger/messenger_idl");

my $PubTest = new PerlDDS::Process_Java ("InternalThreadStatusPublisher", $pub_test_opts,
    ["$DDS_ROOT/java/tests/messenger/messenger_idl/messenger_idl_test.jar"], $vmargs);

my $SubTest = new PerlDDS::Process_Java ("InternalThreadStatusSubscriber", $sub_test_opts,
    ["$DDS_ROOT/java/tests/messenger/messenger_idl/messenger_idl_test.jar"], $vmargs);

$SubTest->Spawn();
sleep(1);
$PubTest->Spawn();

my $PubTestResult = $PubTest->WaitKill (20);
if ($PubTestResult != 0) {
    print STDERR "ERROR: test publisher returned $PubTestResult \n";
    $status = 1;
}

my $SubTestResult = $SubTest->WaitKill (10);
if ($SubTestResult != 0) {
    print STDERR "ERROR: test subscriber returned $SubTestResult \n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
