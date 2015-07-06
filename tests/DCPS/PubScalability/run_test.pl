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

my $status=0;

my $num_writers=4;
my $multiple_instance=1;
my $num_instances=1;
my $num_threads_to_write=1;
my $num_writes_per_thread=2000;
my $num_readers=500;

if ($#ARGV > 1) {
    print "Usage: run_test.pl <num_writers> <num_readers>\n";
    print "       <num_writers> is optional and defaults to $num_writers\n";
    print "       <num_readers> is optional and defaults to $num_readers\n";
    print "\n";
    print "Each reader will be in its own process.  The writers \n";
    print "will all be in the same process and write from separate\n";
    print "threads.\n";
    exit -1;
}
if ($#ARGV >= 0) {
    $num_writers = $ARGV[0];
}
if ($#ARGV == 1) {
    $num_readers = $ARGV[1];
}


#Make max_samples_per_instance large enough.
my $max_samples_per_instance= 12345678;
my $history_depth=2000;
my $blocking_write=0;
my $write_delay_msec=0;
my $receive_delay_msec=100;
my $check_data_dropped=0;
my $publisher_running_sec=1500;
my $subscriber_running_sec=30;
my $repo_bit_conf = "-NOBITS";
my $app_bit_conf = "-DCPSBit 0";
my $cfgfile = '';

$num_instances=$num_threads_to_write * $num_writers;

my $num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

my $dcpsrepo_ior="dcps.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO=PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      " $repo_bit_conf -o $dcpsrepo_ior");
print $DCPSREPO->CommandLine(), "\n";

my $publisher=PerlDDS::create_process ("publisher",
                                       " -DCPSDebugLevel 1 -DCPSTransportDebugLevel 1 "
                                       . " $app_bit_conf $cfgfile"
                                       . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                       . " -t $num_threads_to_write"
                                       . " -w $num_writers"
                                       . " -m $multiple_instance"
                                       . " -i $num_writes_per_thread "
                                       . " -n $max_samples_per_instance"
                                       . " -d $history_depth"
                                       . " -l $write_delay_msec"
                                       . " -r $check_data_dropped "
                                       . " -b $blocking_write ");

print $publisher->CommandLine(), "\n";

my @subscribers;
for (my $i = 0; $i < $num_readers; $i++) {
  my $subscriber=PerlDDS::create_process ("subscriber",
                                          "$app_bit_conf $cfgfile"
                                          . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                          . " -n $num_writes"
                                          . " -l $receive_delay_msec");

  print $subscriber->CommandLine(), "\n";
  push @subscribers, $subscriber;
}

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


for (my $i = 0; $i < $num_readers; $i++) {
  $subscribers[$i]->Spawn ();
}
$publisher->Spawn ();

my $result=$publisher->WaitKill ($publisher_running_sec);

if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status=1;
}

if ($check_data_dropped == 0) {
    for (my $i = 0; $i < $num_readers; $i++) {
      $result=$subscribers[$i]->WaitKill($subscriber_running_sec);

      if ($result != 0) {
          print STDERR "ERROR: subscriber[$i] returned $result  \n";
          $status=1;
      }
    }
}
else {
    for (my $i = 0; $i < $num_readers; $i++) {
      $result=$subscribers[$i]->TerminateWaitKill(5);

      if ($result != 0) {
          print STDERR "ERROR: subscriber[$i] returned $result\n";
          $status=1;
      }
    }
}

my $ir=$DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status=1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
