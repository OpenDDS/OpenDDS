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

if ($ARGV[0] eq 'restart_sub') {
  # Increase the number of messages so that the publisher will last
  # until the subscriber restarted.
  $num_writes = 20;
  $restart_delay = 10;
  $num_reads_before_crash = 2;
  $num_expected_reads_restart_sub = $num_writes - $num_reads_before_crash;
} 
elsif ($ARGV[0] eq 'restart_pub') { 
  $restart_delay = 10;
  $num_writes_before_crash = 2;
  $num_expected_reads = $num_writes + $num_writes_before_crash;
}
elsif ($ARGV[0] eq 'bp_timeout') { 
  # no delay between writes.
  $write_delay_ms=0;
  # delay 2 seconds between reads to make the 
  # transport have backpressure.
  $read_delay_ms=2000;
  $num_writes = 10000;
  $num_expected_reads = $num_writes;
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

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;


$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/dds/InfoRepo/DCPSInfoRepo",
				  "-NOBITS -o $dcpsrepo_ior -d $domains_file -ORBSvcConf repo.conf");
$Subscriber = new PerlACE::Process ("subscriber", 
          "-DCPSConfigFile sub.ini -a $num_reads_before_crash -n $num_expected_reads"
          . " -i $read_delay_ms");
$Publisher = new PerlACE::Process ("publisher", 
          "-DCPSConfigFile pub.ini -a $num_writes_before_crash -n $num_writes"
          . " -i $write_delay_ms");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$Publisher->Spawn ();

$Subscriber->Spawn ();

# The subscriber crashes and we need restart the subscriber.
if ($num_reads_before_crash > 0)
{
  $SubscriberResult = $Subscriber->WaitKill (60);
  
  if ($SubscriberResult == 0)
  {
    print STDERR "ERROR: Subscriber crashed and returned 0. \n";
    $status = 1;
  }
  else
  {
    print STDERR "Subscriber crashed and returned $SubscriberResult. \n";
  }
  
  $Subscriber = new PerlACE::Process ("subscriber", 
          "-DCPSConfigFile sub.ini -n $num_expected_reads_restart_sub -r");
  
  sleep($restart_delay);
         
  print STDERR "\n\n!!! Restart subscriber !!! \n\n";;
  $Subscriber->Spawn ();
}

# The publisher crashes and we need restart the publisher.
if ($num_writes_before_crash > 0) {
  $PublisherResult = $Publisher->WaitKill (60);
  
  if ($PublisherResult == 0)
  {
    print STDERR "ERROR: Publisher crashed and returned 0. \n";
    $status = 1;
  }
  else
  {
    print STDERR "Publisher crashed and returned $PublisherResult. \n";
  }

  $Publisher = new PerlACE::Process ("publisher", 
          "-DCPSConfigFile pub.ini -n $num_writes");
  
  sleep($restart_delay);
    
  print STDERR "\n\n!!! Restart publisher !!! \n\n";;
  $Publisher->Spawn ();
}
  
$SubscriberResult = $Subscriber->WaitKill (300);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$PublisherResult = $Publisher->WaitKill (30);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
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
}

exit $status;
