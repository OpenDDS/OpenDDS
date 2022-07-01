eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

sub run_test {
  my $type = shift;
  my $extensibility = shift;
  my $xcdr = shift;


  my $test_name = "${type}_${extensibility}_XCDR$xcdr";
  my @common_args = (
    "${type}_${extensibility}",
    $xcdr,
    '-DCPSConfigFile rtps_disc.ini',
    '-ORBDebugLevel 10 -DCPSDebugLevel 10',
    '-ORBLogFile',
  );
  my $reader_name = "reader_$test_name";
  my @reader_args = (@common_args, "$reader_name.log");
  my $writer_name = "writer_$test_name";
  my @writer_args = (@common_args, "$writer_name.log");

  my $test = new PerlDDS::TestFramework();
  $test->enable_console_logging();

  $test->process($reader_name, './Recorder/xtypes_dynamic_recorder', join(' ', @reader_args));
  $test->start_process($reader_name);

  $test->process($writer_name, './Pub/xtypes_dynamic_pub', join(' ', @writer_args));
  $test->start_process($writer_name);

  my $failed = 0;
  $failed |= $test->wait_kill($reader_name, 15);
  $failed |= $test->wait_kill($writer_name, 15);
  $failed |= $test->finish(60);
  if ($failed) {
    print STDERR "ERROR: $test_name failed\n";
    return 1;
  }
  return 0;
}

my $total_tests = 0;
my $failed_tests = 0;
my @types = ("my_struct", "outer_struct", "inner_union", "outer_union");
my @extensibilities = ("final", "appendable", "mutable");
my @xcdrs = ("1", "2");

foreach my $type (@types) {
  foreach my $extensibility (@extensibilities) {
    foreach my $xcdr (@xcdrs) {
      if (!($xcdr eq "1" && $extensibility eq "mutable")) {
        $total_tests++;
        $failed_tests++ if (run_test($type, $extensibility, $xcdr));
      }
    }
  }
}

print "$total_tests tests ran\n";
if ($failed_tests) {
  print STDERR "ERROR: $failed_tests test(s) failed\n";
}

exit($failed_tests ? 1 : 0);
