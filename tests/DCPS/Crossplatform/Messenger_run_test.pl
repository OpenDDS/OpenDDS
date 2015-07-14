eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Cross_Sync;
use strict;

$| = 1;
my $status = 0;

my $file_prefix = "$DDS_ROOT/tests/DCPS/Messenger";
my $pub_config_file = "$file_prefix/pub.ini";
my $sub_config_file = "$file_prefix/sub.ini";
my $opts = "";
my $pub_opts = "";
my $sub_opts = "";

if ($ARGV[0] ne 'tcp') {
  my $use_pubsub = ($ARGV[0] !~ /^rtps/);
  $pub_config_file = "$file_prefix/" . ($use_pubsub ? 'pub_' : '') .
      "$ARGV[0].ini";
  $sub_config_file = "$file_prefix/" . ($use_pubsub ? 'sub_' : '') .
      "$ARGV[0].ini";
}

my $CS = new PerlDDS::Cross_Sync(1, PerlACE::random_port(),
                                 PerlACE::random_port(), $pub_config_file,
                                 $sub_config_file);
if (!$CS) {
  print "Crossplatform test pre-reqs not met. Skipping...\n";
  exit 0;
}

my @test_configs = $CS->get_config_info();
$pub_config_file = $test_configs[0];
$sub_config_file = $test_configs[1];

my $role = $CS->wait();
if ($role == -1) {
  print "ERROR: Test pre-reqs not met.\n";
  exit -1;
}

my @ports = $CS->boot_ports();
my $port1 = 10001 + $ports[0];
my $dcpsrepo_ior = 'repo.ior';
my $repo_host;
if ($role == PerlDDS::Cross_Sync_Common::SERVER) {
  $repo_host = $CS->self();
} else {
  $repo_host = $CS->peer();
}

my $common_args = $opts;
if ($ARGV[0] !~ /^rtps_disc/) {
  $common_args .= " -DCPSInfoRepo corbaloc:iiop:$repo_host:$port1/DCPSInfoRepo";
}

unlink $dcpsrepo_ior;

PerlACE::add_lib_path("$DDS_ROOT/tests/DCPS/Messenger");

my $Subscriber =
    PerlDDS::create_process("$DDS_ROOT/tests/DCPS/Messenger/subscriber",
                            "-DCPSConfigFile $sub_config_file $common_args " .
                            $sub_opts);
my $Publisher =
    PerlDDS::create_process("$DDS_ROOT/tests/DCPS/Messenger/publisher",
                            "-DCPSConfigFile $pub_config_file $common_args " .
                            $pub_opts);
my $DCPSREPO = undef;
if ($role == PerlDDS::Cross_Sync_Common::SERVER) {

  if ($ARGV[0] !~ /^rtps_disc/) {
    unlink $dcpsrepo_ior;
    $DCPSREPO = PerlDDS::create_process("$DDS_ROOT/bin/DCPSInfoRepo",
                                        "-o $dcpsrepo_ior " .
                                        "-ORBListenEndpoints iiop://:$port1");

    print $DCPSREPO->CommandLine() . "\n";
    $DCPSREPO->Spawn();
    if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
      print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
      $DCPSREPO->Kill();
      exit 1;
    }
    unlink $dcpsrepo_ior;
  }

  print $Publisher->CommandLine() . "\n";
  $Publisher->Spawn();

  if ($CS->ready() == -1) {
    print STDERR "ERROR: subscriber failed to initialize.\n";
    $status = 1;
    $DCPSREPO->Kill() if defined $DCPSREPO;
    $Publisher->Kill();
    exit 1;
  }

  my $PublisherResult = $Publisher->WaitKill(300);
  if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
  }
} else {
  print $Subscriber->CommandLine() . "\n";
  $Subscriber->Spawn();

  my $SubscriberResult = $Subscriber->WaitKill(15);
  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
  }
}

$role = $CS->wait();
if ($role == -1) {
  print "ERROR: Shutdown sync failed.\n";
  $status = 1;
  $DCPSREPO->Kill() if defined $DCPSREPO;
  exit 1;
}

if ($role == PerlDDS::Cross_Sync_Common::SERVER) {
  if ($CS->ready() == -1) {
    print STDERR "ERROR: subscriber failed to finish sync properly.\n";
    $status = 1;
  }
  my $ir = (defined $DCPSREPO) ? $DCPSREPO->TerminateWaitKill(5) : 0;
  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;

#  LocalWords:  eval
