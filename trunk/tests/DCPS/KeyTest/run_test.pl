eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
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

my $LONE_PROCESS = 1; #only one "other" process runs at a time
$TST = PerlDDS::create_process("KeyMarshaling", "", $LONE_PROCESS);
print STDERR "Running KeyMarshaling\n";
my $retcode = $TST->SpawnWaitKill(60);
if ($retcode != 0) {
    $status = 1;
}

$TST = PerlDDS::create_process("KeyTest_MD5", "", $LONE_PROCESS);
print STDERR "Running KeyTest_MD5\n";
my $retcode = $TST->SpawnWaitKill(60);
if ($retcode != 0) {
    $status = 1;
}


$TST = PerlDDS::create_process("IsBounded", "", $LONE_PROCESS);
print STDERR "Running IsBounded\n";
my $retcode = $TST->SpawnWaitKill(60);
if ($retcode != 0) {
    $status = 1;
}


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

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
