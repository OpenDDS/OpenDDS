eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../FooType');

my $test_opts;
if ($ARGV[0] eq 'rtps') {
  shift;
  $test_opts = '-DCPSConfigFile rtps.ini';
}

$test_opts .= " @ARGV";

my $status = 0;

my $orig_ACE_LOG_TIMESTAMP = $ENV{ACE_LOG_TIMESTAMP};
$ENV{ACE_LOG_TIMESTAMP} = "TIME";
sub cleanup
{
  $ENV{ACE_LOG_TIMESTAMP} = $orig_ACE_LOG_TIMESTAMP;
}

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");
$test_opts .= " -DCPSDebugLevel 1";
my $Test = PerlDDS::create_process("test", $test_opts);

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    cleanup();
    exit 1;
}

print $Test->CommandLine() . "\n";
$Test->Spawn();

my $TestResult = $Test->WaitKill(300);
if ($TestResult != 0) {
  print STDERR "ERROR: test returned $TestResult \n";
  $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

cleanup();
exit $status;
