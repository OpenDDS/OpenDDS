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

my $test = new PerlDDS::TestFramework();

$test->{'dcps_debug_level'} = 0;
$test->{'dcps_transport_debug_level'} = 0;
$test->setup_discovery();

my $pub_count = 0;
my $sub_count = 0;
my $rtps_disc = 0;
my $large_samples = 0;

my $ai = 0;
foreach $a(@ARGV) {
  if ($a eq "rtps_disc") {
    $rtps_disc = 1;
  }
  if ($a eq "rtps_disc_tcp") {
    $rtps_disc = 1;
  }
  if ($a eq "large_samples") {
    $large_samples = 1;
  }
  if ($a eq "publishers") {
    $pub_count = @ARGV[$ai + 1];
  }
  if ($a eq "subscribers") {
    $sub_count = @ARGV[$ai + 1];
  }
  $ai++;
}

if ($pub_count == 0) {
  $pub_count = 10;
}

if ($sub_count == 0) {
  $sub_count = 10;
}

my $total_count = $pub_count + $sub_count;

my $pub_index = 1;
my $sub_index = 1;

my $pub_args = "";
if ($large_samples) {
  $pub_args = $pub_args . " -l";
}
$pub_args = $pub_args . " -DCPSPendingTimeout 3";

my $sub_args = "";
$sub_args = $sub_args . " -DCPSPendingTimeout 3";

for (my $i = 0; $i < $total_count; $i++) {
  if (0 == $i % 2) {
    if ($pub_index <= $pub_count) {
      $test->process("pub_" . $pub_index++, "publisher", $pub_args);
    } elsif ($sub_index <= $sub_count) {
      $test->process("sub_" . $sub_index++, "subscriber", $sub_args);
    }
  } else {
    if ($sub_index <= $sub_count) {
      $test->process("sub_" . $sub_index++, "subscriber", $sub_args);
    } elsif ($pub_index <= $pub_count) {
      $test->process("pub_" . $pub_index++, "publisher", $pub_args);
    }
  }
}

$pub_index = 1;
$sub_index = 1;

for (my $i = 0; $i < $total_count; $i++) {
  if (0 == $i % 2) {
    if ($pub_index <= $pub_count) {
      $test->start_process("pub_" . $pub_index++);
    } elsif ($sub_index <= $sub_count) {
      $test->start_process("sub_" . $sub_index++);
    }
  } else {
    if ($sub_index <= $sub_count) {
      $test->start_process("sub_" . $sub_index++);
    } elsif ($pub_index <= $pub_count) {
      $test->start_process("pub_" . $pub_index++);
    }
  }
}

exit $test->finish(600);
