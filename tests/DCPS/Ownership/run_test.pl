eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

$debuglevel = 0;
$debug_opts = "-ORBDebugLevel $debuglevel -ORBVerboseLogging 1 -DCPSDebugLevel $debuglevel -DCPSTransportDebugLevel $debuglevel";

$is_rtps_disc = 0;

$pub_opts = "$debug_opts";
$sub_opts = "$debug_opts";
$shutdown_pub = 0;
$sub_deadline = "";
$pub1_deadline = "";
$pub2_deadline = "";
$pub1_reset_strength = "";
$sub_liveliness = "";
$pub1_liveliness = "";
$pub2_liveliness = "";
$testcase = 0;

if ($ARGV[0] eq 'liveliness_change') {
    $sub_liveliness = "-l 2";
    $pub1_liveliness = "-l 1 -y 250";
    $pub2_liveliness = "-l 1 -y 250 -c";
    $testcase = 1;
}
elsif ($ARGV[0] eq 'miss_deadline') {
    $sub_deadline = "-d 2";
    $pub1_deadline = "-d 1 -y 500";
    $pub2_deadline = "-d 1 -y 500 -c";
    $testcase = 2;
}
elsif ($ARGV[0] eq 'update_strength') {
    $pub1_reset_strength = "-r 15";
    $testcase = 3;
}
elsif ($ARGV[0] eq 'rtps') {
    $is_rtps_disc = 1;
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

if ($#ARGV > 0) {
    if ($ARGV[1] eq 'rtps') {
	$is_rtps_disc = 1;
    }
    else {
	print STDERR "ERROR: invalid test case\n";
	exit 1;
    }
}

if ($is_rtps_disc) {
    $pub_opts .= " -DCPSConfigFile rtps.ini";
    $sub_opts .= " -DCPSConfigFile rtps.ini";
}
else {
    $pub_opts .= " -DCPSConfigFile pub.ini";
    $sub_opts .= " -DCPSConfigFile sub.ini";
}


$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

unless ($is_rtps_disc) {
    $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
			 "$debug_opts -ORBLogFile DCPSInfoRepo.log -o $dcpsrepo_ior ");
}

$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts -ORBLogFile sub.log $sub_deadline $sub_liveliness -t $testcase");

$Publisher1 = PerlDDS::create_process ("publisher", "$pub_opts -ORBLogFile pub1.log -s 10 -i datawriter1 $pub1_reset_strength $pub1_deadline $pub1_liveliness");
$Publisher2 = PerlDDS::create_process ("publisher", "$pub_opts -ORBLogFile pub2.log -s 12 -i datawriter2 $pub2_deadline $pub2_liveliness");

unless ($is_rtps_disc) {
    print $DCPSREPO->CommandLine() . "\n";
    $DCPSREPO->Spawn ();
    if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
	print STDERR "ERROR: waiting for Info Repo IOR file\n";
	$DCPSREPO->Kill ();
	exit 1;
    }
}

print $Publisher1->CommandLine() . "\n";
$Publisher1->Spawn ();


print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

if ($testcase == 0) {
  sleep (3);
}

print $Publisher2->CommandLine() . "\n";
$Publisher2->Spawn ();


$PublisherResult = $Publisher1->WaitKill (60);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher 1 returned $PublisherResult\n";
    $status = 1;
}


$PublisherResult = $Publisher2->WaitKill (60);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher 2 returned $PublisherResult\n";
    $status = 1;
}


$SubscriberResult = $Subscriber->WaitKill (60);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

unless ($is_rtps_disc) {
    $ir = $DCPSREPO->TerminateWaitKill(5);
    if ($ir != 0) {
	print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
	$status = 1;
    }
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
