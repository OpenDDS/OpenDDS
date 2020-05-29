eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use FileHandle;
use Cwd;

sub compiler_test {
  # These files each represent an expected error from the key processing
  # code in the opendds_idl compiler.  Run the compiler against each of
  # them and verify that an error occurs.
  my @error_files = ("KeyTypeError_array_noindex.idl",
                     "KeyTypeError_bad_nesting.idl",
                     "KeyTypeError_missing_right_bracket.idl",
                     "KeyTypeError_multi_dim_array.idl",
                     "KeyTypeError_nofield.idl",
                     "KeyTypeError_nonarrayindex2.idl",
                     "KeyTypeError_nonarrayindex.idl",
                     "KeyTypeError_struct_no_nest.idl",
                     );
  foreach my $file (@error_files) {
    my $idl = PerlDDS::create_process ("$ENV{DDS_ROOT}/dds/idl/opendds_idl",
                                       "--default-nested $file",
                                       $LONE_PROCESS);
    open(SAVEOUT, ">&STDOUT");
    open(SAVEERR, ">&STDERR");
    open(STDOUT, '>' . File::Spec->devnull());
    open(STDERR, ">&STDOUT");
    my $idl_ret = $idl->SpawnWaitKill(30);
    open(STDOUT, ">&SAVEOUT");
    open(STDERR, ">&SAVEERR");
    if ($idl_ret == 0) {
      print STDERR "ERROR: opendds_idl processed $file cleanly when expecting " .
          "error\n";
      return 1;
    }
  }
  return 0;
}

my %framework_tests = (
  'keymarshalling' => 'KeyMarshalling',
  'md5' => 'KeyTest_MD5',
  'isbounded' => 'IsBounded',
);
my @all_tests = keys(%framework_tests);
push(@all_tests, 'compiler');
my %all_tests_hash = map {$_ => 1} @all_tests;

foreach $arg (@ARGV) {
  if (!exists($all_tests_hash{$arg})) {
    die("Invalid argument: $arg\n");
  }
}

my $failed = 0;
foreach $testname (scalar(@ARGV) ? @ARGV : @all_tests) {
  if ($testname eq 'compiler') {
    $failed |= compiler_test();
  } else {
    my $test = new PerlDDS::TestFramework();
    $test->process($testname, $framework_tests{$testname}, $LONE_PROCESS);
    $test->start_process($testname);
    $failed |= $test->finish(30) ? 1 : 0;
  }
}
exit($failed);
