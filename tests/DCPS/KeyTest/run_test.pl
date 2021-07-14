eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

sub compiler_test {
  # These files each represent an expected error from the key processing
  # code in the opendds_idl compiler.  Run the compiler against each of
  # them and verify that an error occurs.
  my @error_files = (
    "invalid_array_noindex.idl",
    "invalid_bad_nesting.idl",
    "invalid_missing_right_bracket.idl",
    "invalid_multi_dim_array.idl",
    "invalid_nofield.idl",
    "invalid_nonarrayindex2.idl",
    "invalid_nonarrayindex.idl",
    "invalid_struct_no_nest.idl",
    "invalid_sequence.idl",
  );
  my $failed = 0;
  foreach my $file (@error_files) {
    my $opendds_idl = PerlDDS::get_opendds_idl();
    if (!defined($opendds_idl)) {
      return 1;
    }
    my $cmd = "$opendds_idl --default-nested $file";
    print("compiler_test: $cmd\n");
    my $found_error = 0;
    unless (open(FH, "$cmd 2>&1 |")) {
      print STDERR "ERROR: Couldn't run $cmd: $!\n";
      return 1;
    }
    while (<FH>) {
      $found_error = 1 if /^Error - /;
    }
    my $error_status = close(FH);
    unless ($found_error && !$error_status) {
      print STDERR "ERROR: opendds_idl processed $file cleanly when expecting " .
          "error (found error: $found_error, \$?: $?)\n";
      $failed = 1;
    }
  }
  return $failed;
}

sub command {
  my $exe = shift;
  return sub {
    my $name = shift;
    my $test = new PerlDDS::TestFramework();
    $test->process($name, $exe);
    $test->start_process($name);
    return $test->finish(30) ? 1 : 0;
  };
}

my @all_tests = (
  ['keymarshalling', command('KeyMarshalling')],
  ['md5', command('KeyTest_MD5')],
  ['isbounded', command('IsBounded')],
  ['compiler', \&compiler_test],
);
my @all_test_names = map { $_->[0] } @all_tests;
my %all_tests_hash = map { $_->[0] => $_->[1] } @all_tests;

foreach my $arg (@ARGV) {
  if (!exists($all_tests_hash{$arg})) {
    die("Invalid argument: $arg\n");
  }
}

my $failed = 0;
foreach my $testname (scalar(@ARGV) ? @ARGV : @all_test_names) {
  $failed |= $all_tests_hash{$testname}->($testname);
}
exit($failed);
