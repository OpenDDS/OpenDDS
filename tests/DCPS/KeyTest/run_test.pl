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

my $status = 0;
my $test = new PerlDDS::TestFramework();
my $valid;

if ($test->flag('keymarshalling')) {
  $test->process('Test', 'KeyMarshalling', $LONE_PROCESS);
  $valid = 1;
} elsif ($test->flag('md5')) {
  $test->process('Test', 'KeyTest_MD5', $LONE_PROCESS);
  $valid = 1;
} elsif ($test->flag('isbounded')) {
  $test->process('Test', 'IsBounded', $LONE_PROCESS);
  $valid = 1;
} elsif ($test->flag('compiler')) {
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
                                       $file,
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
      $status = 1;
    }
  }
}

if ($valid) {
  $test->start_process('Test');
  sleep 5;
  $status = $test->finish(30);
}

exit $status;
