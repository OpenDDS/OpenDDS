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

my $status = 0;

PerlDDS::add_lib_path('../ManyTopicTypes');
PerlDDS::add_lib_path('../common');

my $subscriber1_completed = "T1_subscriber_finished.txt";
my $subscriber2_completed = "T2_subscriber_finished.txt";
my $subscriber3_completed = "T3_subscriber_finished.txt";

my $publisher1_completed = "T1_publisher_finished.txt";
my $publisher2_completed = "T2_publisher_finished.txt";
my $publisher3_completed = "T3_publisher_finished.txt";

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

my $sub_parameters = "-t all";
my $pub_parameters = "-t all";

my $test = new PerlDDS::TestFramework();
if ($rtps_disc) {
  $sub_parameters .= ' -DCPSConfigFile rtps_disc.ini';
  $pub_parameters .= ' -DCPSConfigFile rtps_disc.ini';
}
else {
  $test->setup_discovery();
}

$test->process("subscriber1", "subscriber", $sub_parameters);
$test->process("subscriber2", "subscriber", $sub_parameters);
$test->process("publisher", "publisher", $pub_parameters);

$test->add_temporary_file("subscriber1", $subscriber1_completed);
$test->add_temporary_file("subscriber1", $subscriber2_completed);
$test->add_temporary_file("subscriber1", $subscriber3_completed);

$test->add_temporary_file("publisher", $publisher1_completed);
$test->add_temporary_file("publisher", $publisher2_completed);
$test->add_temporary_file("publisher", $publisher3_completed);

$test->start_process("publisher", "-o");
$test->start_process("subscriber1", "-o");
$test->start_process("subscriber2", "-o");

exit $test->finish(300);
