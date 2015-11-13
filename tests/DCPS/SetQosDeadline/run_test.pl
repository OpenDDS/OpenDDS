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
my $is_rtps_disc = 0;

if ($ARGV[0] eq 'rtps_disc') {
  $is_rtps_disc = 1;
}

my $pub_opts = "-DCPSConfigFile " . ($is_rtps_disc ? "rtps_disc.ini" : "pub.ini");
my $sub_opts = "-DCPSConfigFile " . ($is_rtps_disc ? "rtps_disc.ini" : "sub.ini");

my $test = new PerlDDS::TestFramework();
$test->setup_discovery() unless $is_rtps_disc;
$test->process("subscriber", "subscriber", $sub_opts);
$test->process("publisher", "publisher", $pub_opts);

my $synch_file = "dr_unmatch_done";
unlink $synch_file;

$test->start_process("publisher");
$test->start_process("subscriber");

my $result = $test->finish(300);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
  $status = 1;
}

unlink $synch_file;

exit $status;
