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
  my $test_name_param = "stru";
  my @reader_args = ("$test_name_param -DCPSConfigFile rtps_disc.ini -ORBLogFile recorder_$test_name_param.log -ORBDebugLevel 10 -DCPSDebugLevel 10");
  my @writer_args = ("$test_name_param -DCPSConfigFile rtps_disc.ini -ORBLogFile publisher_$test_name_param.log -ORBDebugLevel 10 -DCPSDebugLevel 10");

  $test->process("reader_$test_name_param", './Recorder/xtypes_dynamic_recorder', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  $test->process("writer_$test_name_param", './Pub/xtypes_dynamic_pub', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");

  $status |= $test->wait_kill("reader_$test_name_param", 15);
  $status |= $test->wait_kill("writer_$test_name_param", 15);

  my $test_name_param = "nested";
  my @reader_args = ("$test_name_param -DCPSConfigFile rtps_disc.ini -ORBLogFile recorder_$test_name_param.log -ORBDebugLevel 10 -DCPSDebugLevel 10");
  my @writer_args = ("$test_name_param -DCPSConfigFile rtps_disc.ini -ORBLogFile publisher_$test_name_param.log -ORBDebugLevel 10 -DCPSDebugLevel 10");

  $test->process("reader_$test_name_param", './Recorder/xtypes_dynamic_recorder', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  $test->process("writer_$test_name_param", './Pub/xtypes_dynamic_pub', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");

  $status |= $test->wait_kill("reader_$test_name_param", 15);
  $status |= $test->wait_kill("writer_$test_name_param", 15);

  my $test_name_param = "union";
  my @reader_args = ("$test_name_param -DCPSConfigFile rtps_disc.ini -ORBLogFile recorder_$test_name_param.log -ORBDebugLevel 10 -DCPSDebugLevel 10");
  my @writer_args = ("$test_name_param -DCPSConfigFile rtps_disc.ini -ORBLogFile publisher_$test_name_param.log -ORBDebugLevel 10 -DCPSDebugLevel 10");

  $test->process("reader_$test_name_param", './Recorder/xtypes_dynamic_recorder', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  $test->process("writer_$test_name_param", './Pub/xtypes_dynamic_pub', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");

  $status |= $test->wait_kill("reader_$test_name_param", 15);
  $status |= $test->wait_kill("writer_$test_name_param", 15);
}
run_test();
if ($status) {
  print STDERR "ERROR: test failed\n";
}

exit $test->finish(60);
