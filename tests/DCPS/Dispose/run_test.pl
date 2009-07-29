eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

PerlDDS::add_lib_path('../FooType');

$opts = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf tcp.conf";
$test_opts = "$opts ";

$status = 0;

$dcpsrepo_ior = "repo.ior";
$repo_bit_opt = $opts;

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$repo_bit_opt -o $dcpsrepo_ior ");

$Test = PerlDDS::create_process("test", "$test_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Test->CommandLine() . "\n";
$Test->Spawn();

$TestResult = $Test->WaitKill(300);
if ($TestResult != 0) {
  print STDERR "ERROR: test returned $TestResult \n";
  $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
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

exit $status;
