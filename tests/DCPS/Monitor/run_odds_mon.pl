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
my $use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');

my $opts = $use_svc_config ? "-ORBSvcConf monitor.conf" : '';
my $repo_bit_opt = $opts;

my $pub_opts = "$opts -ORBDebugLevel 10 -ORBLogFile pub.log -DCPSConfigFile pub.ini -DCPSDebugLevel 10";
my $sub_opts = "$opts -DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile sub.log -DCPSConfigFile sub.ini -DCPSDebugLevel 10";
my $mon_opts = "-DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile mon.log -DCPSDebugLevel 10 -DCPSConfigFile sub.ini";

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink qw/pub.log sub.log mon.log DCPSInfoRepo.log/;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-DCPSDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

my $Monitor    = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/monitor", " $mon_opts");
my $Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
my $Publisher  = PerlDDS::create_process ("publisher",  " $pub_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Monitor->CommandLine() . "\n";
$Monitor->Spawn ();

sleep(10);

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


my $PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

my $MonitorResult = $Monitor->WaitKill(100000);
if ($MonitorResult != 0) {
    print STDERR "ERROR: Monitor returned $MonitorResult\n";
    $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
