eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Sys::Hostname;

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;
use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use CrossSyncDDS;

$| = 1;
my $status = 0;

my $pub_config_file = "$ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/pub.ini";
my $sub_config_file = "$ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/sub.ini";
$CS = new CrossSyncDDS (1, PerlACE::uniqueid(), PerlACE::uniqueid()
			, $pub_config_file, $sub_config_file);
if (!$CS) {
    print "Crossplatform test pre-reqs not met. Skipping...\n";
    exit 0;
}

@test_configs = $CS->get_config_info();
$pub_config_file = @test_configs[0];
$sub_config_file = @test_configs[1];

my $role = $CS->wait();
if ($role == -1) {
    print "ERROR: Test pre-reqs not met.\n";
    exit -1;
}

my $svc_conf = '';
my $repo_bit_opt = '';
if (!new PerlACE::ConfigList->check_config ('STATIC')) {
  $repo_bit_opt = "-ORBSvcConf $ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/tcp.conf";
  if ($ARGV[0] eq 'udp') {
    $svc_conf = " -ORBSvcConf $ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/udp.conf ";
  }
  else {
    $svc_conf = " -ORBSvcConf $ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/tcp.conf";
  }
}

@ports = $CS->boot_ports ();
my($port1) = 10001 + @ports[0];
my $domains_file = "$ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/domain_ids";
my $dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
my $repo_host;
if ($role == CrossSync::SERVER) {
    $repo_host = $CS->self();
} else {
    $repo_host = $CS->peer();
}
my $common_args = "-DCPSInfoRepo corbaloc:iiop:$repo_host:$port1/DCPSInfoRepo"
    . " $svc_conf";

unlink $dcpsrepo_ior;

$Subscriber = new PerlACE::Process
    ("$ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/subscriber",
     "-DCPSConfigFile $sub_config_file $common_args");
#$Subscriber = new PerlACE::Process ("subscriber",
#                                    "-DCPSConfigFile $sub_config_file $common_args");
$Publisher = new PerlACE::Process
    ("$ENV{DDS_ROOT}/DevGuideExamples/DCPS/Messenger/publisher",
     "-DCPSConfigFile $pub_config_file $common_args");

if ($role == CrossSync::SERVER) {
    unlink $dcpsrepo_ior;
    $DCPSREPO = new PerlACE::Process
	("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
	 "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file "
	 . "-ORBEndpoint iiop://$repo_host:$port1");

    print $DCPSREPO->CommandLine(). "\n";
    $DCPSREPO->Spawn ();
    if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
	print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
	$DCPSREPO->Kill ();
	exit 1;
    }
    unlink $dcpsrepo_ior;

    print $Publisher->CommandLine(). "\n";
    $Publisher->Spawn ();

    if ($CS->ready () == -1) {
	print STDERR "ERROR: subscriber failed to initialize.\n";
	$status = 1;
	$DCPSREPO->Kill ();
	$Publisher->Kill ();
	exit 1;
    }

    $PublisherResult = $Publisher->WaitKill (300);
    if ($PublisherResult != 0) {
	print STDERR "ERROR: publisher returned $PublisherResult \n";
	$status = 1;
    }

    $ir = $DCPSREPO->TerminateWaitKill(5);
    if ($ir != 0) {
	print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
	$status = 1;
    }
} else {
    print $Subscriber->CommandLine(). "\n";
    $Subscriber->Spawn ();

    $SubscriberResult = $Subscriber->WaitKill (15);
    if ($SubscriberResult != 0) {
	print STDERR "ERROR: subscriber returned $SubscriberResult \n";
	$status = 1;
    }
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
