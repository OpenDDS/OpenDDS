eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

$opts = new PerlACE::ConfigList->check_config ('STATIC')
    ? '' : '-ORBSvcConf ../../tcp.conf';

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
$repo_bit_opt = $opts eq '' ? '' : '-ORBSvcConf tcp.conf';
$info_prst_file = PerlACE::LocalFile ("info.pr");
$num_messages = 60;
$pub_opts = "$opts -DCPSConfigFile pub.ini -n $num_messages";
$sub_opts = "$opts -DCPSConfigFile sub.ini -n $num_messages";

unlink $dcpsrepo_ior;
unlink $info_prst_file;

if (PerlACE::is_vxworks_test()) {
  # If InfoRepo is running in persistent mode, use a
  #  static endpoint (instead of transient)
  $DCPSREPO = new PerlACE::ProcessVX ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file "
                                      . "-ORBSvcConf mySvc.conf "
                                      . "-orbendpoint iiop://:12345");
  $Subscriber = new PerlACE::ProcessVX ("subscriber", " $sub_opts");
  $Publisher = new PerlACE::ProcessVX ("publisher", " $pub_opts");
  $Monitor1 = new PerlACE::ProcessVX ("monitor", " $opts -l 7");
  $Monitor2 = new PerlACE::ProcessVX ("monitor", " $opts -u");
}
else {
  # If InfoRepo is running in persistent mode, use a
  #  static endpoint (instead of transient)
  $DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file "
                                    #. "-ORBDebugLevel 10 "
                                    . "-ORBSvcConf mySvc.conf "
                                    . "-orbendpoint iiop://:12345");
  $Subscriber = new PerlACE::Process ("subscriber", " $sub_opts");
  $Publisher = new PerlACE::Process ("publisher", " $pub_opts");
  $Monitor1 = new PerlACE::Process ("monitor", " $opts -l 7");
  $Monitor2 = new PerlACE::Process ("monitor", " $opts -u");
}

$data_file = PerlACE::LocalFile ("test_run_prst.data");
unlink $data_file;

print $DCPSREPO->CommandLine() . "\n";
print $Publisher->CommandLine() . "\n";
print $Subscriber->CommandLine() . "\n";
print $Monitor1->CommandLine() . "\n";
print $Monitor2->CommandLine() . "\n";


open (OLDOUT, ">&STDOUT");
open (STDOUT, ">$data_file") or die "can't redirect stdout: $!";
open (OLDERR, ">&STDERR");
open (STDERR, ">&STDOUT") or die "can't redirect stderror: $!";

print "Spawning DCPSInfoRepo.\n";

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print "Spawning first monitor.\n";

$Monitor1->Spawn ();

print "Spawning publisher.\n";

$Publisher->Spawn ();

print "Spawning subscriber.\n";

$Subscriber->Spawn ();
 
sleep (15);

print "Killing first DCPSInfoRepo.\n";

$ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}
unlink $dcpsrepo_ior;
 
print "Spawning second DCPSInfoRepo.\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

sleep (15);

print "Spawning second monitor.\n";

$Monitor2->Spawn ();

$MonitorResult = $Monitor1->WaitKill (20);
if ($MonitorResult != 0) {
    print STDERR "ERROR: Monitor1 returned $MonitorResult \n";
    $status = 1;
}

$MonitorResult = $Monitor2->WaitKill (300);
if ($MonitorResult != 0) {
    print STDERR "ERROR: Monitor2 returned $MonitorResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->TerminateWaitKill (10);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$PublisherResult = $Publisher->TerminateWaitKill (10);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

close (STDERR);
close (STDOUT);
open (STDOUT, ">&OLDOUT");
open (STDERR, ">&OLDERR");

unlink $dcpsrepo_ior;
#unlink $data_file;
unlink $info_prst_file;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
