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

PerlDDS::add_lib_path('../FooType5');

# single reader with single instances test

my $num_writers = 1;
my $num_instances_per_writer = 1;
my $num_samples_per_instance = 100;
my $use_take = 0;
my $use_udp = 0;
my $use_multicast = 0;
my $use_rtps_transport = 0;
my $num_readers = 1;
my $max_samples_per_instance = 1000;
my $sequence_length = 10;
my $no_key = 0;
my $write_interval_ms = 0;
my $writer_blocking_ms = 0;
my $read_interval_ms = 0;
my $mixed_trans = 0;

my $arg_idx = 0;

if ($ARGV[$arg_idx] eq 'nokey') {
  $no_key = 1;
  ++$arg_idx;
}

if ($ARGV[$arg_idx] eq 'udp') {
  $use_udp = 1;
  ++$arg_idx;
  $write_interval_ms = 50;
}

if ($ARGV[$arg_idx] eq 'multicast') {
  $use_multicast = 1;
  ++$arg_idx;
  $write_interval_ms = 50;
}

if ($ARGV[$arg_idx] eq 'rtps') {
  $use_rtps_transport = 1;
  ++$arg_idx;
  $write_interval_ms = 50;
}

# multiple instances, single datawriter, single datareader
if ($ARGV[$arg_idx] eq 'mi') {
  $num_instances_per_writer = 2;
}
elsif ($ARGV[$arg_idx] eq 'mwmr') {
  $num_samples_per_instance = 50;
  $num_writers = 2;
  $num_readers = 2;
}
elsif ($ARGV[$arg_idx] eq 'mwmr_long_seq') {
  $sequence_length = 256;
  $num_samples_per_instance = 50;
  $num_writers = 2;
  $num_readers = 2;
}
elsif ($ARGV[$arg_idx] eq 'blocking') {
  $writer_blocking_ms = 1000000; # 1000 seconds
  $num_instances_per_writer = 5;
  $num_samples_per_instance = 20;
  $max_samples_per_instance = 1;
  $read_interval_ms = 1000;
}
elsif ($ARGV[$arg_idx] eq 'blocking_timeout') {
  $writer_blocking_ms = 5; # milliseconds
  $num_instances_per_writer = 5;
  $num_samples_per_instance = 50;
  $max_samples_per_instance = 1;
  $sequence_length = 1000;
  $read_interval_ms = 1000;
}
elsif ($ARGV[$arg_idx] eq 'mixed_trans') {
  $num_samples_per_instance = 50;
  $num_writers = 2;
  $num_readers = 2;
  $mixed_trans = 1;
}
elsif ($ARGV[$arg_idx] eq '') {
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[$arg_idx]\n";
  exit 1;
}

my $dcpsrepo_ior = 'repo.ior';

my $subscriber_completed = 'subscriber_finished.txt';
my $subscriber_ready = 'subscriber_ready.txt';
my $publisher_completed = 'publisher_finished.txt';
my $publisher_ready = 'publisher_ready.txt';

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;

my $DCPSREPO = PerlDDS::create_process("$DDS_ROOT/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");
print $DCPSREPO->CommandLine(), "\n";

my $sub_parameters = "-DCPSConfigFile all.ini -u $use_udp -c $use_multicast"
    . " -p $use_rtps_transport -r $num_readers -t $use_take"
    . " -m $num_instances_per_writer -i $num_samples_per_instance"
    . " -w $num_writers -z $sequence_length"
    . " -k $no_key -y $read_interval_ms -f $mixed_trans";

my $pub_parameters = "-DCPSConfigFile all.ini -u $use_udp -c $use_multicast "
    . " -p $use_rtps_transport -w $num_writers "
    . " -m $num_instances_per_writer -i $num_samples_per_instance "
    . " -n $max_samples_per_instance -z $sequence_length"
    . " -k $no_key -y $write_interval_ms -b $writer_blocking_ms"
    . " -f $mixed_trans";

my $Subscriber = PerlDDS::create_process('subscriber', $sub_parameters);
print $Subscriber->CommandLine(), "\n";

my $Publisher = PerlDDS::create_process('publisher', $pub_parameters);
print $Publisher->CommandLine(), "\n";

$DCPSREPO->Spawn();

if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
  $DCPSREPO->Kill();
  exit 1;
}


$Publisher->Spawn();

$Subscriber->Spawn();

my $status = 0;
my $PublisherResult = $Publisher->WaitKill(300);

if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult\n";
  $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill(15);

if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult\n";
  $status = 1;
}

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
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
