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

my $publisher1_completed = "T1_publisher_finished.txt";
my $publisher2_completed = "T2_publisher_finished.txt";
my $publisher3_completed = "T3_publisher_finished.txt";

unlink $subscriber1_completed;
unlink $subscriber2_completed;
unlink $subscriber3_completed;

unlink $publisher1_completed;
unlink $publisher2_completed;
unlink $publisher3_completed;

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

my $sub_parameters = "-t all";
my $pub_parameters = "-t all";

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");

if ($rtps_disc) {
  $sub_parameters .= ' -DCPSConfigFile rtps_disc.ini';
  $pub_parameters .= ' -DCPSConfigFile rtps_disc.ini';
}
else {
  $DCPSREPO->Spawn();

  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
  }
}

my $Subscriber = PerlDDS::create_process("subscriber", $sub_parameters);
my $Sub2 = PerlDDS::create_process("subscriber", $sub_parameters);
my $Publisher = PerlDDS::create_process("publisher", $pub_parameters);

$Publisher->Spawn();
$Subscriber->Spawn();
$Sub2->Spawn();

my $PublisherResult = $Publisher->WaitKill(300);
if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult\n";
  $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill(30);
if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult\n";
  $status = 1;
}

my $Sub2Result = $Sub2->WaitKill(5);
if ($Sub2Result != 0) {
  print STDERR "ERROR: subscriber2 returned $Sub2Result\n";
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

unlink $publisher1_completed;
unlink $publisher2_completed;
unlink $publisher3_completed;

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
