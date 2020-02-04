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

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $send_interval = 0;
my $test_duration = 300;
if ($ARGV[0] eq 'long') {
  $send_interval = 30;
  $test_duration = 600;
}

my $status = 0;

my $pub_opts = "-i $send_interval -ORBDebugLevel 10 -ORBLogFile pub.log -DCPSConfigFile pub.ini -DCPSDebugLevel 10";
my $sub_opts = "-DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile sub.log -DCPSConfigFile sub.ini -DCPSDebugLevel 10";
my $mon_opts = "-DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile mon.log -DCPSConfigFile mon.ini -DCPSDebugLevel 10";

my $dcpsrepo_ior = "repo.ior";
my $monready_file = "mon_ready.txt";

unlink $dcpsrepo_ior;
unlink $monready_file;
unlink qw/pub.log sub.log mon.out mon.log DCPSInfoRepo.log/;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-NOBITS -DCPSDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log -o $dcpsrepo_ior ");

my $Monitor    = PerlDDS::create_process ("monitor",    " $mon_opts");
my $Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
my $Publisher  = PerlDDS::create_process ("publisher",  " $pub_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Monitor->CommandLine() . "\n";
# Redirect std out so we can check it for monitor messages
open(SAVEOUT, ">&STDOUT");
open(STDOUT, '>mon.out');
$Monitor->Spawn ();
open(STDOUT, ">&SAVEOUT");
if (PerlACE::waitforfile_timed ($monready_file, 30) == -1) {
    print STDERR "ERROR: waiting for monitor initialization\n";
    $DCPSREPO->Kill ();
    $Monitor->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


my $PublisherResult = $Publisher->WaitKill ($test_duration);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

my $MonitorResult = $Monitor->TerminateWaitKill(10);
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
unlink $monready_file;

open(MONOUT,"mon.out");
my @monout=<MONOUT>;close MONOUT;
my $mon_count = grep /Report:/,@monout;
print STDOUT "mon_count=$mon_count\n";
if ($mon_count < 58) {
    print STDERR "ERROR: Insufficient number of monitor messages seen\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
