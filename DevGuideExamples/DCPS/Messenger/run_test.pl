eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use Getopt::Long;
use PerlDDS::Run_Test;
use strict;

my $status = 0;
my $rtps = 0;
my $help = 0;

my $dbg_opts = " -ORBDebugLevel 1";
my $common_opts = "$dbg_opts";

my $help_message = "usage: run_test.pl [-h|--help] [--rtps]\n";
my $invalid_args = not GetOptions(
  "rtps" => \$rtps,
  "help|h" => \$help,
);

if (scalar(grep {length($_)} @ARGV)) {
  print STDERR ("ERROR: Invalid positional argument(s) passed: ", join(' ', @ARGV), "\n");
  $invalid_args = 1;
}

if ($invalid_args) {
  print STDERR ("ERROR: Invalid Command Line Argument(s)\n$help_message");
  exit 1;
}

if ($help) {
  print $help_message;
  exit 0;
}

if ($rtps) {
  $common_opts .= " -DCPSConfigFile rtps.ini";
}

my $test = new PerlDDS::TestFramework();

$test->{dcps_debug_level} = 4;
$test->{dcps_transport_debug_level} = 2;
$test->{add_transport_config} = 0;

my $pub_opts = "$common_opts";
my $sub_opts = "$common_opts";

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log") unless $rtps;

$test->process("publisher", "publisher", $pub_opts);
$test->process("subscriber", "subscriber", $sub_opts);

$test->start_process("subscriber");
$test->start_process("publisher");

exit $test->finish(120);
