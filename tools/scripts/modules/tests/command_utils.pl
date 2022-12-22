#!/usr/bin/perl

use strict;
use warnings;

use B qw/perlstring/;
use File::Temp;

use FindBin;
use lib "$FindBin::RealBin/..";
use command_utils;

my $test_exit_status = 0;

sub repr {
  my $x = shift();
  return defined($x) ? perlstring($x) : 'undef';
}

sub check_value {
  my $what = shift();
  my $expected = shift();
  my $actual = shift();

  my $expected_repr = repr($expected);
  my $actual_repr = repr($actual);

  my $not_equal = 0;
  if (defined($expected) && defined($actual)) {
    $not_equal = $expected ne $actual;
  }
  if (defined($expected) != defined($actual) || $not_equal) {
    print STDERR "ERROR: expected $what to be $expected_repr, but it's $actual_repr\n";
    $test_exit_status = 1;
  }
}

sub check_boolean {
  my $what = shift();
  my $expected = shift() ? "true" : "false";
  my $actual = shift() ? "true" : "false";

  check_value($what, $expected, $actual);
}

my $cmd_name = 'test-command';

sub run_command {
  my $expected_failure = shift();
  my $expected_exit_status = shift();
  my $exit_status;
  my $command = shift();
  check_boolean('failure', $expected_failure,
    command_utils::run_command($command,
      verbose => 1,
      error_fh => undef,
      exit_status => \$exit_status,
      name => $cmd_name,
      @_,
    ));
  check_value('exit status', $expected_exit_status, $exit_status);
}

sub perl {
  my $perl_source = shift();
  return [$^X, '-e', $perl_source];
}

print("Check that return value and exit status work as expected ======================\n");
# TODO: This is not consistent under Windows, can report as exit status 1
if ($^O ne 'MSWin32') {
  run_command(1, undef, '___this_really_should_be_invalid___');
}
run_command(1, 2, perl('exit(2);'));
run_command(0, 0, perl('exit(0);'));

print("Check that putting the ouput in a variable works ==============================\n");
my $stdout;
my $expected_stdout = "Hello\n";
my $stdout_perl = 'print(' . perlstring($expected_stdout) . ');';
run_command(0, 0, perl($stdout_perl), capture => {stdout => \$stdout});
check_value('captured stdout', $expected_stdout, $stdout);

my $stderr;
my $expected_stderr = "Goodbye\n";
my $stderr_perl = 'print STDERR (' . perlstring($expected_stderr) . ');';
run_command(0, 0, perl($stderr_perl), capture => {stderr => [\$stderr]});
check_value('captured stderr', $expected_stderr, $stderr);

my $output_perl = $stderr_perl . $stdout_perl;
run_command(0, 0, perl($output_perl), capture => {stdout => [\$stdout], stderr => \$stderr});
check_value('captured stdout', $expected_stdout, $stdout);
check_value('captured stderr', $expected_stderr, $stderr);

my $output;
run_command(0, 0, perl($output_perl), capture => \$output);
check_value('captured all output', $expected_stderr . $expected_stdout, $output);

print("Check that putting the output in a file works =================================\n");
{
  my ($tmp_stdout_fh, $tmp_stdout_path) = File::Temp::tempfile(UNLINK => 1);
  my ($tmp_stderr_fh, $tmp_stderr_path) = File::Temp::tempfile(UNLINK => 1);
  run_command(0, 0, perl($output_perl),
    capture => {
      stdout => $tmp_stdout_fh,
      stderr => [$tmp_stderr_path],
    },
  );
  seek($tmp_stdout_fh, 0, 0) or die("seek failed: $!");
  check_value('capture stdout', $expected_stdout, do { local $/; <$tmp_stdout_fh> });
  close($expected_stdout);
  seek($tmp_stderr_fh, 0, 0) or die("seek failed: $!");
  check_value('capture stderr', $expected_stderr, do { local $/; <$tmp_stderr_fh> });
  close($expected_stderr);
}

print("Check that dump_on_failure works ==============================================\n");
my $output_fail_perl = $output_perl . 'exit(1);';
{
  my ($tmp_fd, $tmp_path) = File::Temp::tempfile(UNLINK => 1);
  run_command(1, 1, perl($output_fail_perl),
    capture => {
      stdout => [\$stdout, dump_on_failure => 1],
      stderr => [\$stderr, dump_on_failure => 1],
    },
    error_fh => $tmp_fd,
  );
  seek($tmp_fd, 0, 0) or die("seek failed: $!");
  my $expected_dump = command_utils::get_dump_output($cmd_name, ['stderr'], \$expected_stderr) .
    command_utils::get_dump_output($cmd_name, ['stdout'], \$expected_stdout);
  # Cut off exit status error
  my $dump = substr((do { local $/; <$tmp_fd> }), 0, length($expected_dump));
  check_value('capture dump', $expected_dump, $dump);
  close($tmp_fd);
}

exit($test_exit_status);
