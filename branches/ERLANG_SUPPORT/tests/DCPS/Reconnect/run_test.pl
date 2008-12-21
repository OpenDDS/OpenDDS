eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

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
$verify_lost_sub_notification = 1;
$pub_port = PerlACE::random_port();
$sub_port = PerlACE::random_port(); 
$pub_local_address = "localhost:$pub_port";
$sub_local_address = "localhost:$sub_port";

if ($ARGV[0] eq 'restart_sub') {
  # Increase the number of messages so that the publisher will last
  # until the subscriber restarted.
  $num_writes = 20;
  $restart_delay = 10;
  $num_reads_before_crash = 2;
  
  #The number of lost message depends on the period between sub crash and 
  #restart.
  #$num_lost_messages_estimate = $restart_delay * 1000/$write_delay_ms;
  #$num_expected_reads_restart_sub
  #  = $num_writes - $num_reads_before_crash - $num_lost_messages_estimate;
  $lost_publication_callback = 1;

  # We can not give an exact number of reads we expected when subscriber crashes
  # because we do not know how many messages are lost. We give a deviation on the
  # number of expected messages.
  $num_reads_deviation = 2;
  $expected_deleted_connections = 2;
}
elsif ($ARGV[0] eq 'dl_clean') {

  $num_writes = 40;
  $restart_delay = 10;
  $num_reads_before_crash = 2;
  $lost_publication_callback = 1;
  $num_reads_deviation = 2;
  $expected_deleted_connections = 1;
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
  $verify_lost_sub_notification = 0;

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

$dcpsrepo_ior = "repo.ior";
$subscriber_completed = "subscriber_finished.txt";
$subscriber_ready = "subscriber_ready.txt";
$publisher_completed = "publisher_finished.txt";
$publisher_ready = "publisher_ready.txt";
$testoutputfilename = "test.log";

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;
unlink $testoutputfilename;

# Save the output to check after execution
open(SAVEERR, ">&STDERR");
open(STDERR, ">$testoutputfilename") || die "ERROR: Can't redirect stderr";

$svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : " -ORBSvcConf ../../tcp.conf ";

$DCPSREPO = PerlDDS::create_process
      ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo"
       , "$svc_config -o $dcpsrepo_ior -ORBSvcConf repo.conf");
$Subscriber = PerlDDS::create_process
      ("subscriber"
       , " $svc_config -a $num_reads_before_crash"
       . " -n $num_expected_reads -i $read_delay_ms -l $lost_subscription_callback"
       . " -c $verify_lost_sub_notification -e $end_with_publisher -x $sub_local_address");
$Publisher = PerlDDS::create_process
      ("publisher"
       , " $svc_config -a $num_writes_before_crash"
       . " -n $num_writes -i $write_delay_ms -l $lost_publication_callback"
       . " -d $expected_deleted_connections -x $pub_local_address");

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
  if (PerlACE::waitforfileoutput_timed ($testoutputfilename, "Subscriber crash after", 90) == -1) {
    close(STDERR);
    open(STDERR, ">&SAVEERR");
    print STDERR "ERROR: waiting for 'Subscriber crash after' output.\n";
    $Subscriber->Kill ();
    $Publisher->Kill ();
    $DCPSREPO->Kill ();
    exit 1;
  }

  #get time at crash
  my $crash_at = time();

  $SubscriberResult = $Subscriber->WaitKill (60);

  # We will not check the status returned from WaitKill() since it returns
  # different status on windows and linux.
  print "Subscriber crashed and returned $SubscriberResult. \n";
  
  sleep($restart_delay);
  
  #get time before restart so we can calculate the lost messages during sub
  #crash and restart.
  my $now = time();
  my $lost_time = $now - $crash_at;
  $num_lost_messages_estimate = $lost_time * 1000/$write_delay_ms;
  $num_expected_reads_restart_sub
    = $num_writes - $num_reads_before_crash - $num_lost_messages_estimate;

  $Subscriber = PerlDDS::create_process
        ("subscriber"
         , " $svc_config -n $num_expected_reads_restart_sub"
         . " -r $num_reads_deviation -x $sub_local_address");

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

  $Publisher = PerlDDS::create_process
        ("publisher"
         , " $svc_config -n $num_writes");

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

# give InfoRepo a chance to digest the publisher crash.
sleep (5);
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
