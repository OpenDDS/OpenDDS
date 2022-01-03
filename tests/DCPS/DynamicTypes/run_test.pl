eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use Getopt::Long;
use strict;

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();

my $status = 0;

sub run_test {
  my @test_name_params = ("my_struct", "outer_struct", "inner_union", "outer_union");
  my @test_extensibilities = ("_final", "_appendable", "_mutable");
  my @xcdr_version_params = ("1", "2");
  foreach my $test_name_param(@test_name_params) {
    foreach my $test_extensibility(@test_extensibilities) {
      foreach my $xcdr_version_param(@xcdr_version_params) {
        if ($xcdr_version_param != "1" || $test_extensibility != "_mutable") {
          my $reader_name = "reader_$test_name_param" . $test_extensibility . "_XCDR$xcdr_version_param";
          my $writer_name = "writer_$test_name_param" . $test_extensibility . "_XCDR$xcdr_version_param";
          my @reader_args = ($test_name_param . "$test_extensibility $xcdr_version_param -DCPSConfigFile rtps_disc.ini -ORBLogFile recorder_$reader_name.log -ORBDebugLevel 10 -DCPSDebugLevel 10");
          my @writer_args = ($test_name_param . "$test_extensibility $xcdr_version_param -DCPSConfigFile rtps_disc.ini -ORBLogFile publisher_$writer_name.log -ORBDebugLevel 10 -DCPSDebugLevel 10");
          $test->process($reader_name, './Recorder/xtypes_dynamic_recorder', join(' ', @reader_args));
          $test->start_process("reader_$test_name_param" . "$test_extensibility" . "_XCDR$xcdr_version_param");

          $test->process($writer_name, './Pub/xtypes_dynamic_pub', join(' ', @writer_args));
          $test->start_process("writer_$test_name_param" . "$test_extensibility" . "_XCDR$xcdr_version_param");

          $status |= $test->wait_kill("reader_$test_name_param" . "$test_extensibility" . "_XCDR$xcdr_version_param", 15);
          $status |= $test->wait_kill("writer_$test_name_param" . "$test_extensibility" . "_XCDR$xcdr_version_param", 15);
        }
      }
    }
  }
}

run_test();

if ($status) {
  print STDERR "ERROR: test failed\n";
}

exit $test->finish(60);
