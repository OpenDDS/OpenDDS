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
use Getopt::Long;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $status = 0;

my $force_local;
my $dcps_dbl = 2;
my $transport_dbl = 0;
my $print_to_screen = 1;
my $help;

my $localhost = "127.0.0.1";

my $help_message = "prst_repo_run_test.pl options:\n"
  . "  --force_local\n"
  . "    Force DCPS to use the localhost address: $localhost\n"
  . "  --DCPSDebugLevel LEVEL\n"
  . "    Enable DCPS Debugging, where LEVEL is 0 up to 10. Default is $dcps_dbl.\n"
  . "  --TransportDebugLevel LEVEL\n"
  . "    Enable DCPS Debugging, where LEVEL is 0 up to 6. Default is $transport_dbl.\n"
  . "  --help|-h\n"
  . "    Print this message.\n"
;

GetOptions(
  "force_local" => \$force_local,
  "DCPSDebugLevel=i"=> \$dcps_dbl,
  "TransportDebugLevel=i"=> \$transport_dbl,
  "help|h" => \$help
) or die("Invalid Command Line Argument(s)\n$help_message");

if ($help) {
  print $help_message;
  exit 0;
}

my @common_array = ();
push @common_array, "-DCPSDefaultAddress $localhost" if $force_local;
push @common_array, "-DCPSDebugLevel $dcps_dbl" if $dcps_dbl;
push @common_array, "-DCPSTransportDebugLevel $transport_dbl" if $transport_dbl;
my $common_args = join(" ", @common_array);

my $client_args = "$common_args";

my $dcpsrepo_ior = "repo.ior";
my $info_prst_file = "info.pr";
my $pub1_msg_count = 30;
my $pub2_msg_count = 10;
my $sub_msg_count = $pub1_msg_count + $pub2_msg_count - 5;
my $pub_opts = "$client_args";
my $sub_opts = "$client_args -n $sub_msg_count";
my $SRV_PORT = PerlACE::random_port();
my $synch_file = "monitor1_done";

my $orb_address = $force_local ? $localhost : "";
my $repo_args = "$common_args"
  . " -o $dcpsrepo_ior"
  . " -ORBSvcConf mySvc.conf"
  . " -orbendpoint iiop://$orb_address:$SRV_PORT"
  . " -DCPSConfigFile info_repo.ini"
;

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

# This is a workaround for TerminateWaitKill method, using the repoctl tool.
#
# On Windows PerlACE uses the Win32::Process Perl module, which, when killing
# a process always does an equivalent of kill -9 (SIGKILL) on processes
# instead of Ctrl-C (SIGINT) that would happen when using TerminateWaitKill on
# Linux. The difference is that the repo will shutdown properly with SIGINT
# and using SIGKILL kills it immediately. At the time of writing this causes
# the test to pass on Windows and fail on Linux.
sub shutdown_repo {
  my $repo = shift;
  my $RepoShutdown = PerlDDS::create_process(
    "$ENV{DDS_ROOT}/bin/repoctl", "kill file://$dcpsrepo_ior"
  );
  print $RepoShutdown->CommandLine() . "\n";

  my $repoctl_result = $RepoShutdown->Spawn();
  if ($repoctl_result) {
    print STDERR "ERROR: repoctl couldn't be started.\n";
  }

  # Make sure repo is killed. This also updates RUNNING even if everything
  # went ok, else it will complain that it is still running when the test
  # ends.
  my $repo_result = $repo->WaitKill(10);
  if ($repo_result > 0) {
    print STDERR "ERROR: repo returned $repo_result\n";
  }

  if (!$repoctl_result) {
    $repoctl_result = $RepoShutdown->WaitKill(10);
    if ($repoctl_result > 0) {
      print STDERR "ERROR: Could not shutdown repo, repoctl returned $repoctl_result.\n";
    }
  }

  return $repoctl_result || $repo_result;
}

# If InfoRepo is running in persistent mode, use a
#  static endpoint (instead of transient)
my $Repo1 = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
  "$repo_args" . ($print_to_screen ? "" : " -ORBLogFile $repo1_log")
);
my $Repo2 = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
  "$repo_args" . ($print_to_screen ? "" : " -ORBLogFile $repo2_log")
);
my $Subscriber1 = PerlDDS::create_process("subscriber",
  "$sub_opts" . ($print_to_screen ? "" : " -ORBLogFile $sub_log")
);
my $Publisher1 = PerlDDS::create_process("publisher",
  "$pub_opts -n $pub1_msg_count" . ($print_to_screen ? "" : " -ORBLogFile $pub1_log")
);
my $Monitor1 = PerlDDS::create_process("monitor",
  "$common_args -l 5" . ($print_to_screen ? "" : " -ORBLogFile $mon1_log")
);
my $Monitor2 = PerlDDS::create_process("monitor",
  "$common_args -u" . ($print_to_screen ? "" : " -ORBLogFile $mon2_log")
);
my $Publisher2 = PerlDDS::create_process("publisher",
  "$pub_opts -n $pub2_msg_count" . ($print_to_screen ? "" : " -ORBLogFile $pub2_log")
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

if (shutdown_repo($Repo1)) {
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
if ($MonitorResult > 0) {
  print STDERR "ERROR: Monitor1 returned $MonitorResult\n";
}
$status = 1 if $MonitorResult;

$MonitorResult = $Monitor2->WaitKill (300);
if ($MonitorResult > 0) {
  print STDERR "ERROR: Monitor2 returned $MonitorResult\n";
}
$status = 1 if $MonitorResult;

my $pub2_started = 0;
if (!$status) {
  print "Spawning second publisher.\n";
  print $Publisher2->CommandLine() . "\n";
  $Publisher2->Spawn ();
  $pub2_started = 1;

  sleep (20);
}

my $SubscriberResult = $Subscriber1->WaitKill (60);
if ($SubscriberResult > 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult\n";
}
$status = 1 if $SubscriberResult;

my $PublisherResult = $Publisher1->TerminateWaitKill (10);
if ($PublisherResult > 0) {
  print STDERR "ERROR: publisher returned $PublisherResult\n";
}
$status = 1 if $PublisherResult;

if ($pub2_started) {
  my $Publisher2Result = $Publisher2->TerminateWaitKill (10);
  if ($Publisher2Result > 0) {
    print STDERR "ERROR: publisher 2 returned $Publisher2Result\n";
  }
  $status = 1 if $Publisher2Result;
}

$status = 1 if shutdown_repo($Repo2);

cleanup();

if ($status) {
  print_logs();
  print STDERR "test FAILED.\n";
} else {
  print "test PASSED.\n";
}

exit $status;
