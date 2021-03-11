eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";

use Getopt::Long;
use PerlDDS::Run_Test;

use strict;

my $status = 0;

my $info_repo = 0;
my $help = 0;

my $help_message = "usage: run_test.pl [-h|--help] [--info-repo]\n";
if (not GetOptions(
  "info-repo" => \$info_repo,
  "help|h" => \$help
)) {
  print STDERR ("Invalid Command Line Argument(s)\n$help_message");
}
if ($help) {
  print $help_message;
  exit 0;
}

unlink "subscriber.log";
unlink "publisher.log";

my $common_opts = "-ORBDebugLevel 10 -DCPSDebugLevel 10";

if ($info_repo) {
  $common_opts .= " -DCPSConfigFile info-repo.ini";
}

my $pub_opts = "$common_opts -ORBLogFile publisher.log";
my $sub_opts = "$common_opts -DCPSTransportDebugLevel 6 " .
               "-ORBLogFile subscriber.log";

my $DCPSREPO;
my $dcpsrepo_ior = "repo.ior";

my $subdir = $PerlACE::Process::ExeSubDir;
my $filename = "subscriber";
my $filename_exe = "subscriber.exe";
if (!(-e $subdir.$filename) && !(-e $subdir.$filename_exe)) {
    print STDERR "ERROR: subscriber does not exist. Subdir: $subdir\n";
    exit 1;
}
$filename = 'publisher';
$filename_exe = "publisher.exe";
if (!(-e $subdir.$filename) && !(-e $subdir.$filename_exe)) {
    print STDERR "ERROR: publisher does not exist. Subdir: $subdir\n";
    exit 1;
}

my $Subscriber = PerlDDS::create_process("subscriber", " $sub_opts");
my $Publisher = PerlDDS::create_process("publisher", " $pub_opts");

if ($info_repo) {
  unlink $dcpsrepo_ior;

  $DCPSREPO = PerlDDS::create_process(
    "$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
    "-ORBDebugLevel 10 " .
    "-ORBLogFile DCPSInfoRepo.log " .
    "-o $dcpsrepo_ior");

  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn();
  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
  }
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();

my $PublisherResult = $Publisher->WaitKill(300);
if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult\n";
  $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill(15);
if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult\n";
  $status = 1;
}

if ($info_repo) {
  my $ir = $DCPSREPO->TerminateWaitKill(5);
  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }

  unlink $dcpsrepo_ior;
}

exit $status;
