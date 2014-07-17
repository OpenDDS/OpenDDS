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

PerlDDS::add_lib_path('../../Utils/');

my $status=0;

# process control parameters
my $num_instances=1;
my $num_writers=1;

# const process control parameters
my $keyed_data=1;
my $multiple_instance=0;
my $num_threads_to_write=5;
my $num_writes_per_thread=2;
my $max_samples_per_instance= 12345678;
my $history_depth=100;
my $blocking_write=0;
my $write_delay_msec=0;
my $receive_delay_msec=0;

# script control parameters
my $check_data_dropped=0;
my $publisher_running_sec=60;
my $subscriber_running_sec=30;
my $repo_bit_conf = "-NOBITS";
my $app_bit_conf = "-DCPSBit 0";
my $cfgfile = '';
my $nokey = 0;

if ($#ARGV >= 0) {
  for (my $i = 0; $i <= $#ARGV; $i++) {
    if ($ARGV[$i] eq '-num_instances') {
      $i++;
      $num_instances = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-num_writers') {
      $i++;
      $num_writers = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-keyed_data') {
      $i++;
      $keyed_data = $ARGV[$i];
    }
    else {
      print STDERR "ERROR: invalid parameter $ARGV[$i] \n";
      exit 1;
    }
  }
}

my $num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

my $dcpsrepo_ior="dcps.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO=PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "$repo_bit_conf -o $dcpsrepo_ior");
print $DCPSREPO->CommandLine(), "\n";

my $publisher=PerlDDS::create_process ("publisher",
                                       "$app_bit_conf $cfgfile"
                                       . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                       . " -keyed_data $keyed_data"
                                       . " -num_writers $num_writers"
                                       . " -history_depth $history_depth"
                                       . " -num_threads_to_write $num_threads_to_write"
                                       . " -multiple_instances $multiple_instance"
                                       . " -num_writes_per_thread $num_writes_per_thread "
                                       . " -max_samples_per_instance $max_samples_per_instance"
                                       . " -write_delay_msec $write_delay_msec"
                                       #. " -r $check_data_dropped "
                                       #. " -b $blocking_write "
);

print $publisher->CommandLine(), "\n";

my $subscriber=PerlDDS::create_process ("subscriber",
                                        "$app_bit_conf $cfgfile"
                                        . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                       . " -keyed_data $keyed_data"
                                        . " -num_writes $num_writes"
                                        . " -receive_delay_msec $receive_delay_msec");

print $subscriber->CommandLine(), "\n";

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$subscriber->Spawn ();
$publisher->Spawn ();

my $result=$publisher->WaitKill ($publisher_running_sec);

if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status=1;
}

if ($check_data_dropped == 0) {
    $result=$subscriber->WaitKill($subscriber_running_sec);

    if ($result != 0) {
        print STDERR "ERROR: $subscriber returned $result  \n";
        $status=1;
    }
}
else {
    $result=$subscriber->TerminateWaitKill(5);

    if ($result != 0) {
        print STDERR "ERROR: subscriber returned $result\n";
        $status=1;
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
