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

my $status = 0;

# Debug Levels
my $dcps_dbl = 2;
my $transport_dbl = 0;

my $num_reads_before_crash = 0;
my $num_writes_before_crash = 0;
my $num_writes = 10;
my $num_expected_reads = $num_writes - $num_reads_before_crash;
my $restart_delay = 0;
my $write_delay_ms = 1000;
my $read_delay_ms=0;
my $lost_publication_callback = 0;
my $lost_subscription_callback = 0;
my $end_with_publisher = 0;
my $sub_init_crash = 0;
my $expected_total_publication_count = 1;
my $verify_lost_sub_notification = 1;

my $num_reads_deviation;

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
  $expected_total_publication_count = 2;
}
elsif ($ARGV[0] eq 'dl_clean') {

  $num_writes = 40;
  $restart_delay = 10;
  $num_reads_before_crash = 2;
  $lost_publication_callback = 1;
  $num_reads_deviation = 2;
  $expected_total_publication_count = 1;
}
elsif ($ARGV[0] eq 'restart_pub') {
  $restart_delay = 10;
  $num_writes_before_crash = 2;
  $num_expected_reads = $num_writes + $num_writes_before_crash;
  $lost_subscription_callback = 1;
  $expected_total_publication_count = 1;
}
elsif ($ARGV[0] eq 'bp_timeout') {
  # no delay between writes.
  $write_delay_ms=0;
  # delay 2 seconds between reads to make the
  # transport have backpressure.
  $read_delay_ms=5000;
  $num_writes = 10000;
  $num_expected_reads = $num_writes;
  $lost_publication_callback = 0;
  $verify_lost_sub_notification = 0;

  $end_with_publisher = 1;
}
elsif ($ARGV[0] eq 'sub_init_crash') {
  $sub_init_crash = 1;
}
elsif ($ARGV[0] eq '') {
  #default test - no reconnect
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0]\n";
  exit 1;
}

my $common_args = "";
$common_args .= "-DCPSDebugLevel $dcps_dbl" if $dcps_dbl;
$common_args .= " " if $dcps_dbl && $transport_dbl;
$common_args .= "-TransportDebugLevel $transport_dbl" if $transport_dbl;
my $client_args = "-DCPSConfigFile opendds.ini $common_args";

my $dcpsrepo_ior = "repo.ior";
my $subscriber_completed = "subscriber_finished.txt";
my $subscriber_ready = "subscriber_ready.txt";
my $publisher_completed = "publisher_finished.txt";
my $publisher_ready = "publisher_ready.txt";
my $repo_log = 'repo.log';
my $pub1_log = 'publisher1.log';
my $pub2_log = 'publisher2.log';
my $sub1_log = 'subscriber1.log';
my $sub2_log = 'subscriber2.log';

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;
unlink $repo_log;
unlink $pub1_log;
unlink $pub2_log;
unlink $sub1_log;
unlink $sub2_log;

my $DCPSREPO = PerlDDS::create_process
      ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
       "-o $dcpsrepo_ior"
       . " $common_args -ORBLogFile $repo_log");
my $Subscriber = PerlDDS::create_process
      ("subscriber"
       , "-a $num_reads_before_crash"
       . " -n $num_expected_reads -i $read_delay_ms -l $lost_subscription_callback"
       . " -c $verify_lost_sub_notification -e $end_with_publisher"
       . " $client_args -ORBLogFile $sub1_log");
my $Publisher = PerlDDS::create_process
      ("publisher"
       , "-a $num_writes_before_crash"
       . " -n $num_writes -i $write_delay_ms -l $lost_publication_callback"
       . " -d $expected_total_publication_count"
       . " $client_args -ORBLogFile $pub1_log");

print $DCPSREPO->CommandLine () . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for Info Repo IOR file\n";
  $DCPSREPO->Kill ();
  PerlDDS::print_file($repo_log);
  print STDERR "test FAILED.\n";
  exit 1;
}

print $Subscriber->CommandLine () . "\n";
$Subscriber->Spawn ();

if ($sub_init_crash > 0)
{
  # Simulate a crashed subscriber by killing it.
  sleep (5); # Let subscriber initialize
  $Subscriber->Kill (0); # Crash it.

  print STDOUT "Subscriber crash before Publisher initialization.\n\n";
}

print $Publisher->CommandLine () . "\n";
$Publisher->Spawn ();

my($PublisherResult, $SubscriberResult);

# The subscriber crashes and we need restart the subscriber.
if ($num_reads_before_crash > 0)
{
  if (PerlACE::waitforfileoutput_timed ($sub1_log, "Subscriber crash after", 90) == -1) {
    print STDERR "ERROR: waiting for 'Subscriber crash after' output.\n";
    $Subscriber->Kill ();
    $Publisher->Kill ();
    $DCPSREPO->Kill ();
    PerlDDS::print_file($repo_log);
    PerlDDS::print_file($sub1_log);
    PerlDDS::print_file($pub1_log);
    print STDERR "test FAILED.\n";
    exit 1;
  }

  #get time at crash
  my $crash_at = time();

  $SubscriberResult = $Subscriber->WaitKill (60);

  # We will not check the status returned from WaitKill() since it returns
  # different status on windows and linux.
  print "Subscriber crashed and returned $SubscriberResult.\n";

  sleep($restart_delay);

  #get time before restart so we can calculate the lost messages during sub
  #crash and restart.
  my $now = time();
  my $lost_time = $now - $crash_at;
  my $num_lost_messages_estimate = $lost_time * 1000/$write_delay_ms;
  my $num_expected_reads_restart_sub
    = $num_writes - $num_reads_before_crash - $num_lost_messages_estimate;

  $Subscriber = PerlDDS::create_process
        ("subscriber"
         , "-n $num_expected_reads_restart_sub"
         . " -r $num_reads_deviation"
         . " $client_args -ORBLogFile $sub2_log");

  print "\n\n!!! Restart subscriber !!!\n\n";
  print $Subscriber->CommandLine () . "\n";
  $Subscriber->Spawn ();
}

# The publisher crashes and we need restart the publisher.
if ($num_writes_before_crash > 0) {
  $PublisherResult = $Publisher->WaitKill (60);

  # We will not check the status returned from WaitKill() since it returns
  # different status on windows and linux.
  print "Publisher crashed and returned $PublisherResult.\n";

  $Publisher = PerlDDS::create_process
        ("publisher",
         "-n $num_writes"
         . " $client_args -ORBLogFile $pub2_log");

  sleep($restart_delay);

  print "\n\n!!! Restart publisher !!!\n\n";
  print $Publisher->CommandLine () . "\n";
  $Publisher->Spawn ();
}

if ($sub_init_crash) {
  # Make sure publisher has started
  # Did not use PerlACE::waitforfile_timed because it tests for a
  # nonemtpy file.
  my $i = 0;
  while (1) {
    if (-e $publisher_ready) {
      last;
    }
    if ($i >= 60) {
      print STDERR "ERROR: timedout waiting for publisher to be ready\n";
      $status = 1;
      last;
    }
    $i++;
    sleep (1);
  }
  # Kill the publisher or else it will just wait for subscribers
  # that will never come.
  $Publisher->Kill(0);

} else {
  my $SubscriberResult = $Subscriber->WaitKill (300);
  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
    $status = 1;
  }

  if ($Publisher->WaitKill (60)) {
    print STDERR "ERROR: publisher returned $PublisherResult\n";
    $status = 1;
  }
}

# give InfoRepo a chance to digest the publisher crash.
sleep (5);
my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $status = 1;
}

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
  PerlDDS::print_file($repo_log);
  PerlDDS::print_file($sub1_log);
  PerlDDS::print_file($pub1_log);
  PerlDDS::print_file($sub2_log) if -f $sub2_log;
  PerlDDS::print_file($pub2_log) if -f $pub2_log;
}

exit $status;
