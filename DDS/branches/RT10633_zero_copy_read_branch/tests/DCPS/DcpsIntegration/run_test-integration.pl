eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$testoutputfilename = "test.log";
$status = 0;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");

unlink $dcpsrepo_ior;

PerlACE::add_lib_path('../FooType');

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                            "-o $dcpsrepo_ior"
                            . " -d $domains_file -ORBDebugLevel 1 -NOBITS");


$Test = new PerlACE::Process ("infrastructure_test",
                               "-DCPSInfoRepo file://$dcpsrepo_ior");

# save output to a faile because the output contaings "ERROR"
open(SAVEERR, ">&STDERR");
open(STDERR, ">$testoutputfilename") || die "ERROR: Can't redirect stderr";

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 5) == -1) {
    print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
    exit 1;
}

$TestResult = $Test->SpawnWaitKill (60);

close(STDERR);
open(STDERR, ">&SAVEERR") || die "ERROR: Can't redirect stderr";
close (SAVEERR);

if ($TestResult != 0) {
    print STDERR "ERROR: test returned $TestResult\n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(30);

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
