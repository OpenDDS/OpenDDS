eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

$num_reads_before_crash = 0;
$num_writes_before_crash = 0;
$num_writes = 10;
$num_expected_reads = $num_writes - $num_reads_before_crash;
$restart_delay = 0;
$write_delay_ms = 1000;
$read_delay_ms=0;
$lost_publication_callback = 0;
$lost_subscription_callback = 0;
$end_with_publisher = 0;
$kill_subscriber = 0;
$expected_deleted_connections = 1;

if ($ARGV[0] eq 'restart_sub') {
  # Increase the number of messages so that the publisher will last
  # until the subscriber restarted.
  $num_writes = 20;
  $restart_delay = 10;
  $num_reads_before_crash = 2;
  $num_lost_messages_estimate = $restart_delay * 1000/$write_delay_ms;
  $num_expected_reads_restart_sub
    = $num_writes - $num_reads_before_crash - $num_lost_messages_estimate;
  $lost_publication_callback = 1;

  # We can not give an exact number of reads we expected when subscriber crashes
  # because we do not know how many messages are lost. We give a deviation on the
  # number of expected messages.
  $num_reads_deviation = 2;
  $expected_deleted_connections = 2;
}
elsif ($ARGV[0] eq 'restart_pub') {
  $restart_delay = 10;
  $num_writes_before_crash = 2;
  $num_expected_reads = $num_writes + $num_writes_before_crash;
  $lost_subscription_callback = 1;
}
elsif ($ARGV[0] eq 'bp_timeout') {
  # no delay between writes.
  $write_delay_ms=0;
  # delay 2 seconds between reads to make the
  # transport have backpressure.
  $read_delay_ms=5000;
  $num_writes = 10000;
  $num_expected_reads = $num_writes;
  $lost_publication_callback = 1;
  $lost_subscription_callback = 1;

  $end_with_publisher = 1;
}
elsif ($ARGV[0] eq 'sub_init_crash') {
    $kill_subscriber = 1;
}
elsif ($ARGV[0] eq '') {
  #default test - no reconnect
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0]\n";
  exit 1;
}


$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
$subscriber_completed = PerlACE::LocalFile ("subscriber_finished.txt");
$subscriber_ready = PerlACE::LocalFile ("subscriber_ready.txt");
$publisher_completed = PerlACE::LocalFile ("publisher_finished.txt");
$publisher_ready = PerlACE::LocalFile ("publisher_ready.txt");
$testoutputfilename = PerlACE::LocalFile ("test.log");

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;
unlink $testoutputfilename;

# Save the output to check after execution
open(SAVEERR, ">&STDERR");
open(STDERR, ">$testoutputfilename") || die "ERROR: Can't redirect stderr";

$svc_config=" -ORBSvcConf ../../tcp.conf ";

$DCPSREPO = new PerlACE::Process
    ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo"
     , "-NOBITS -o $dcpsrepo_ior -d $domains_file -ORBSvcConf repo.conf");
$Subscriber = new PerlACE::Process
    ("subscriber"
     , " $svc_config -DCPSConfigFile sub.ini -a $num_reads_before_crash"
     . " -n $num_expected_reads -i $read_delay_ms -l $lost_subscription_callback"
     . " -e $end_with_publisher");
$Publisher = new PerlACE::Process
    ("publisher"
     , " $svc_config -DCPSConfigFile pub.ini -a $num_writes_before_crash"
     . " -n $num_writes -i $write_delay_ms -l $lost_publication_callback"
     . " -d $expected_deleted_connections");

print $DCPSREPO->CommandLine () . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Subscriber->CommandLine () . "\n";
$Subscriber->Spawn ();

if ($kill_subscriber > 0)
{
    # Simulate a crashed subscriber by killing it.
    sleep (5); # Let subscriber initialize
    $Subscriber->Kill (0); # Crash it.

    print STDOUT "Subscriber crash before Publisher initialization.\n\n";
}

print $Publisher->CommandLine () . "\n";
$Publisher->Spawn ();

# The subscriber crashes and we need restart the subscriber.
if ($num_reads_before_crash > 0)
{
  $SubscriberResult = $Subscriber->WaitKill (60);

  # We will not check the status returned from WaitKill() since it returns
  # different status on windows and linux.
  print "Subscriber crashed and returned $SubscriberResult. \n";

  $Subscriber = new PerlACE::Process
      ("subscriber"
       , " $svc_config -DCPSConfigFile sub.ini -n $num_expected_reads_restart_sub"
       . " -r $num_reads_deviation");

  sleep($restart_delay);

  print "\n\n!!! Restart subscriber !!! \n\n";;
  print $Subscriber->CommandLine () . "\n";
  $Subscriber->Spawn ();
}

# The publisher crashes and we need restart the publisher.
if ($num_writes_before_crash > 0) {
  $PublisherResult = $Publisher->WaitKill (60);

  # We will not check the status returned from WaitKill() since it returns
  # different status on windows and linux.
  print "Publisher crashed and returned $PublisherResult. \n";

  $Publisher = new PerlACE::Process
      ("publisher"
       , " $svc_config -DCPSConfigFile pub.ini -n $num_writes");

  sleep($restart_delay);

  print "\n\n!!! Restart publisher !!! \n\n";;
  print $Publisher->CommandLine () . "\n";
  $Publisher->Spawn ();
}

if ($kill_subscriber == 0)
{
    $SubscriberResult = $Subscriber->WaitKill (300);
    if ($SubscriberResult != 0) {
	print STDERR "ERROR: subscriber returned $SubscriberResult \n";
	$status = 1;
    }
}

$PublisherResult = $Publisher->WaitKill (30);
if ($kill_subscriber != 0 && $PublisherResult == 0) {
    # writing out to STDOUT as these tests redirect STDERR to a log file.
    # The nightly script parses STDERR to detect test failures.
    print STDOUT "ERROR: Publisher crashed\n";
    $status = 1;
}
elsif ($kill_subscriber == 0 &&  $PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

# Stop capturing the output
close(STDERR);
open(STDERR, ">&SAVEERR");

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED. Please see the $testoutputfilename for details.\n";
}

exit $status;
