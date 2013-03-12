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

PerlDDS::add_lib_path('../ManyTopicTypes');
PerlDDS::add_lib_path('../common');

my $subscriber1_completed = "T1_subscriber_finished.txt";
my $subscriber2_completed = "T2_subscriber_finished.txt";
my $subscriber3_completed = "T3_subscriber_finished.txt";
my $subscriber4_completed = "T4_subscriber_finished.txt";
my $subscriber5_completed = "T5_subscriber_finished.txt";
my $subscriber6_completed = "T6_subscriber_finished.txt";
my $subscriber7_completed = "T7_subscriber_finished.txt";

my $publisher1_completed = "T1_publisher_finished.txt";
my $publisher2_completed = "T2_publisher_finished.txt";
my $publisher3_completed = "T3_publisher_finished.txt";
my $publisher4_completed = "T4_publisher_finished.txt";
my $publisher5_completed = "T5_publisher_finished.txt";
my $publisher6_completed = "T6_publisher_finished.txt";
my $publisher7_completed = "T7_publisher_finished.txt";

unlink $subscriber1_completed;
unlink $subscriber2_completed;
unlink $subscriber3_completed;
unlink $subscriber4_completed;
unlink $subscriber5_completed;
unlink $subscriber6_completed;
unlink $subscriber7_completed;

unlink $publisher1_completed;
unlink $publisher2_completed;
unlink $publisher3_completed;
unlink $publisher4_completed;
unlink $publisher5_completed;
unlink $publisher6_completed;
unlink $publisher7_completed;

# single reader with single instances test
my $multiple_instance = 0;
my $num_samples_per_reader = 10;
my $num_readers = 1;
my $use_take = 0;

my $use_udp = 0;
my $rtps_disc = 0;

my $arg_idx = 0;

if ($ARGV[0] eq 'udp') {
  $use_udp = 1;
  $arg_idx = 1;
}
elsif ($ARGV[0] eq 'rtps') {
  $rtps_disc = 1;
  $arg_idx = 1;
}

# multiple instances test
if ($ARGV[$arg_idx] eq 'mi') {
  $multiple_instance = 1;
  $num_samples_per_reader = 10;
  $num_readers = 1;
}
# multiple datareaders with single instance test
elsif ($ARGV[$arg_idx] eq 'mr') {
  $multiple_instance = 0;
  $num_samples_per_reader = 5;
  $num_readers = 2;
}
# multiple datareaders with multiple instances test
elsif ($ARGV[$arg_idx] eq 'mri') {
  $multiple_instance = 1;
  $num_samples_per_reader = 4;
  $num_readers = 3;
}
# multiple datareaders with multiple instances test
elsif ($ARGV[$arg_idx] eq 'mrit') {
  $multiple_instance = 1;
  $num_samples_per_reader = 4;
  $num_readers = 3;
  $use_take = 1;
}
elsif ($ARGV[$arg_idx] eq '') {
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[$arg_idx] $arg_idx\n";
  exit 1;
}

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $proc1_parameters = "-p1 -p2 -s6";
my $proc2_parameters = "-p3 -p4 -p5 -s7";
my $proc3_parameters = "-s1 -s2 -s3 -s4 -s5 -p6 -p7";

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");

if ($rtps_disc) {
  $proc1_parameters .= ' -DCPSConfigFile rtps_disc.ini';
  $proc2_parameters .= ' -DCPSConfigFile rtps_disc.ini';
  $proc3_parameters .= ' -DCPSConfigFile rtps_disc.ini';
}
else {
  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn();

  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
  }
}

my $Process1 = PerlDDS::create_process("publisher", $proc1_parameters);
my $Process2 = PerlDDS::create_process("publisher", $proc2_parameters);
my $Process3 = PerlDDS::create_process("subscriber", $proc3_parameters);

print $Process1->CommandLine() . "\n";
$Process1->Spawn();

print $Process2->CommandLine() . "\n";
$Process2->Spawn();

print $Process3->CommandLine() . "\n";
$Process3->Spawn();

my $Process1Result = $Process1->WaitKill(300);
if ($Process1Result != 0) {
  print STDERR "ERROR: process 1 returned $Process1Result\n";
  $status = 1;
}

my $Process2Result = $Process2->WaitKill(30);
if ($Process2Result != 0) {
  print STDERR "ERROR: process 2 returned $Process2Result\n";
  $status = 1;
}

my $Process3Result = $Process3->WaitKill(5);
if ($Process3Result != 0) {
  print STDERR "ERROR: process 3 returned $Process3Result\n";
  $status = 1;
}

if (!$rtps_disc) {
  my $ir = $DCPSREPO->TerminateWaitKill(5);

  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }
}

unlink $subscriber1_completed;
unlink $subscriber2_completed;
unlink $subscriber3_completed;
unlink $subscriber4_completed;
unlink $subscriber5_completed;
unlink $subscriber6_completed;
unlink $subscriber7_completed;

unlink $publisher1_completed;
unlink $publisher2_completed;
unlink $publisher3_completed;
unlink $publisher4_completed;
unlink $publisher5_completed;
unlink $publisher6_completed;
unlink $publisher7_completed;

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
