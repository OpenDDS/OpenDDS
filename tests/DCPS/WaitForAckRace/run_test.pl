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

my $pub_count = 0;
my $sub_count = 0;
my $rtps_disc = 0;
my $large_samples = 0;
my $sub_delay = 0;

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
  if ($a eq "sub_delay") {
    $sub_delay = 1;
  }
#  if ($a eq "publishers") {
#    $pub_count = @ARGV[$ai + 1];
#  }
  if ($a eq "subscribers") {
    $sub_count = @ARGV[$ai + 1];
  }
  $ai++;
}

$test->{'dcps_debug_level'} = 0;
$test->{'dcps_transport_debug_level'} = 0;
$test->setup_discovery() unless $rtps_disc;

if ($pub_count == 0) {
  $pub_count = 1;
}

if ($sub_count == 0) {
  $sub_count = 3;
}

my $pub_index = 1;
my $sub_index = 1;

my $pub_args = "";
my $sub_args = "";

if ($large_samples) {
  $pub_args = $pub_args . " -l";
  $sub_args = $sub_args . " -l";
}
if ($sub_delay) {
  $pub_args = $pub_args . " -d";
  $sub_args = $sub_args . " -d";
}
$pub_args = $pub_args . " -r $sub_count" . " -DCPSPendingTimeout 3 -ORBVerboseLogging 1";

$sub_args = $sub_args . " -DCPSPendingTimeout 3 -ORBVerboseLogging 1";

for (my $i = 0; $i < $pub_count; $i++) {
  $test->process("pub_" . $pub_index++, "publisher", $pub_args);
}

for (my $i = 0; $i < $sub_count; $i++) {
  $test->process("sub_" . $sub_index++, "subscriber", $sub_args);
}

$pub_index = 1;
$sub_index = 1;

for (my $i = 0; $i < $pub_count; $i++) {
  $test->start_process("pub_" . $pub_index++);
}

for (my $i = 0; $i < $sub_count; $i++) {
  sleep(3);
  $test->start_process("sub_" . $sub_index++);
}

exit $test->finish(30);
