eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $dcps_dbl = 2;
my $transport_dbl = 0;

my $common_args = "";
$common_args .= "-DCPSDebugLevel $dcps_dbl" if $dcps_dbl;
$common_args .= " " if $dcps_dbl && $transport_dbl;
$common_args .= "-TransportDebugLevel $transport_dbl" if $transport_dbl;
my $client_args = "$common_args";

my $dcpsrepo_ior = "repo.ior";
my $info_prst_file = "info.pr";
my $num_messages = 60;
my $pub_opts = "$client_args -n $num_messages";
my $num_messages += 10;
my $sub_opts = "$client_args -n $num_messages";
my $SRV_PORT = PerlACE::random_port();
my $synch_file = "monitor1_done";

my $repo_args = "$common_args"
  . " -o $dcpsrepo_ior"
  . " -ORBSvcConf mySvc.conf"
  . " -orbendpoint iiop://:$SRV_PORT";

my $repo1_log = 'repo1.log';
my $repo2_log = 'repo2.log';
my $sub_log = 'subscriber.log';
my $pub1_log = 'publisher1.log';
my $pub2_log = 'publisher2.log';
my $mon1_log = 'monitor1.log';
my $mon2_log = 'monitor2.log';

unlink $repo1_log;
unlink $repo2_log;
unlink $sub_log;
unlink $pub1_log;
unlink $pub2_log;
unlink $mon1_log;
unlink $mon2_log;

sub early_fail() {
  print_logs();
  cleanup();
  print STDERR "test FAILED.\n";
  exit 1;
}

sub cleanup() {
  unlink $dcpsrepo_ior;
  unlink $info_prst_file;
  unlink $synch_file;
}

cleanup();

# If InfoRepo is running in persistent mode, use a
#  static endpoint (instead of transient)
my $Repo1 = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
  "$repo_args -ORBLogFile $repo1_log"
);
my $Repo2 = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
  "$repo_args -ORBLogFile $repo2_log"
);
my $Subscriber1 = PerlDDS::create_process("subscriber",
  "$sub_opts -ORBLogFile $sub_log"
);
my $Publisher1 = PerlDDS::create_process("publisher",
  "$pub_opts -ORBLogFile $pub1_log"
);
my $Monitor1 = PerlDDS::create_process("monitor",
  "$common_args -l 5 -ORBLogFile $mon1_log"
);
my $Monitor2 = PerlDDS::create_process("monitor",
  "$common_args -u -ORBLogFile $mon2_log"
);
my $Publisher2 = PerlDDS::create_process("publisher",
  "$pub_opts -ORBLogFile $pub2_log"
);

sub print_logs() {
  PerlDDS::print_file($repo1_log);
  PerlDDS::print_file($repo2_log);
  PerlDDS::print_file($sub_log);
  PerlDDS::print_file($pub1_log);
  PerlDDS::print_file($pub2_log);
  PerlDDS::print_file($mon1_log);
  PerlDDS::print_file($mon2_log);
}

print "Spawning first DCPSInfoRepo.\n";

print $Repo1->CommandLine() . "\n";
$Repo1->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
  $Repo1->Kill ();
  early_fail();
}

print "Spawning first monitor.\n";

print $Monitor1->CommandLine() . "\n";
$Monitor1->Spawn ();

print "Spawning publisher.\n";

print $Publisher1->CommandLine() . "\n";
$Publisher1->Spawn ();

print "Spawning subscriber.\n";

print $Subscriber1->CommandLine() . "\n";
$Subscriber1->Spawn ();

sleep (15);

print "Killing first DCPSInfoRepo.\n";

my $ir = $Repo1->TerminateWaitKill(10);
if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $Subscriber1->Kill();
  $Publisher1->Kill();
  $Monitor1->Kill();
  early_fail();
}

unlink $dcpsrepo_ior;

print "Spawning second DCPSInfoRepo.\n";
print $Repo2->CommandLine() . "\n";
$Repo2->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
  $Repo2->Kill ();
  $Subscriber1->Kill();
  $Publisher1->Kill();
  $Monitor1->Kill();
  early_fail();
}

sleep (15);

print "Spawning second monitor.\n";

print $Monitor2->CommandLine() . "\n";
$Monitor2->Spawn ();

my $MonitorResult = $Monitor1->WaitKill (20);
if ($MonitorResult != 0) {
  print STDERR "ERROR: Monitor1 returned $MonitorResult \n";
  $status = 1;
}

$MonitorResult = $Monitor2->WaitKill (300);
if ($MonitorResult != 0) {
  print STDERR "ERROR: Monitor2 returned $MonitorResult \n";
  $status = 1;
}

my $pub2_started = 0;
if (!$status) {
  print "Spawning second publisher.\n";
  print $Publisher2->CommandLine() . "\n";
  $Publisher2->Spawn ();
  $pub2_started = 1;

  sleep (5);
}

my $SubscriberResult = $Subscriber1->WaitKill (60);
if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult \n";
  $status = 1;
}

my $PublisherResult = $Publisher1->TerminateWaitKill (10);
if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult \n";
  $status = 1;
}

if ($pub2_started) {
  my $Publisher2Result = $Publisher2->TerminateWaitKill (10);
  if ($Publisher2Result != 0) {
    print STDERR "ERROR: publisher 2 returned $Publisher2Result \n";
    $status = 1;
  }
}

$ir = $Repo2->TerminateWaitKill(10);
if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $status = 1;
}

cleanup();

if ($status) {
  print_logs();
  print STDERR "test FAILED.\n";
} else {
  print "test PASSED.\n";
}

exit $status;
