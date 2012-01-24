eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;
my $is_rtps_disc = 0;
my $DCPScfg = "dcps.ini";
my $DCPSREPO;
my $num_topics = 2;

if ($ARGV[0] eq 'rtps_disc') {
  $DCPScfg = $ARGV[0] . ".ini";
  $is_rtps_disc = 1;
  $num_topics = 0;
}
elsif ($ARGV[0] eq 'rtps_disc_tcp') {
  $DCPScfg = $ARGV[0] . ".ini";
  $is_rtps_disc = 1;
  $num_topics = 0;
}
my $opts = "-DCPSConfigFile $DCPScfg";
my $mon_opts = "-t $num_topics";

my $dcpsrepo_ior = "repo.ior";
unlink $dcpsrepo_ior;

if (!$is_rtps_disc) {
  $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");
}
my $Subscriber = PerlDDS::create_process ("subscriber", "$opts");
my $Publisher = PerlDDS::create_process ("publisher", "$opts");
my $Monitor1 = PerlDDS::create_process ("monitor", "$opts $mon_opts -l 7");
my $Monitor2 = PerlDDS::create_process ("monitor", "$opts $mon_opts -u");
my $synch_file = "monitor1_done";

my $data_file = "test_run.data";
unlink $data_file;
unlink $synch_file;

if (!$is_rtps_disc) {
  print $DCPSREPO->CommandLine() . "\n";
}
print $Publisher->CommandLine() . "\n";
print $Subscriber->CommandLine() . "\n";
print $Monitor1->CommandLine() . "\n";
print $Monitor2->CommandLine() . "\n";


open (OLDOUT, ">&STDOUT");
open (STDOUT, ">$data_file") or die "can't redirect stdout: $!";
open (OLDERR, ">&STDERR");
open (STDERR, ">&STDOUT") or die "can't redirect stderror: $!";

if (!$is_rtps_disc) {
  $DCPSREPO->Spawn ();
  if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
  }
}
$Monitor1->Spawn ();

$Publisher->Spawn ();
$Subscriber->Spawn ();

sleep (15);

$Monitor2->Spawn ();

my $MonitorResult = $Monitor1->WaitKill (300);
if ($MonitorResult != 0) {
    print STDERR "ERROR: Monitor1 returned $MonitorResult \n";
    $status = 1;
}
$MonitorResult = $Monitor2->WaitKill (300);
if ($MonitorResult != 0) {
    print STDERR "ERROR: Monitor2 returned $MonitorResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill (300);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

my $PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

if (!$is_rtps_disc) {
  my $ir = $DCPSREPO->TerminateWaitKill(5);
  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }
}

close (STDERR);
close (STDOUT);
open (STDOUT, ">&OLDOUT");
open (STDERR, ">&OLDERR");

unlink $dcpsrepo_ior;
#unlink $data_file;
unlink $synch_file;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
