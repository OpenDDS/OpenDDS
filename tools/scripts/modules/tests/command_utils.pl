#!/usr/bin/perl

use strict;
use warnings;

use B qw/perlstring/;

use FindBin;
use lib "$FindBin::RealBin/..";
use command_utils;

my $test_exit_status = 0;

sub repr {
  my $x = shift;
  return defined($x) ? perlstring($x) : 'undef';
}

sub check_value {
  my $what = shift;
  my $expected = shift;
  my $actual = shift;

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
  my $what = shift;
  my $expected = shift ? "true" : "false";
  my $actual = shift ? "true" : "false";

  check_value($what, $expected, $actual);
}

sub run_command {
  my $expected_failure = shift;
  my $expected_exit_status = shift;
  my $exit_status;
  check_boolean('failure', $expected_failure,
    command_utils::run_command(@_, print_error => 0, verbose => 1, exit_status => \$exit_status));
  check_value('exit status', $expected_exit_status, $exit_status);
}

sub perl {
  my $perl_source = shift;
  return [$^X, '-e', $perl_source];
}

run_command(1, undef, '___this_really_should_be_invalid___');
run_command(1, 2, perl('exit(2);'));
run_command(0, 0, perl('exit(0);'));

my $stdout;
my $expected_stdout = "Hello\n";
run_command(0, 0, perl('print(' . perlstring($expected_stdout) . ');'), capture_stdout => \$stdout);
check_value('captured stdout', $expected_stdout, $stdout);

exit($test_exit_status);
