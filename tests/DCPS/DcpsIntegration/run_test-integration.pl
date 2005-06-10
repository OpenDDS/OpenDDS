eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use lib '../../../../../bin';
use PerlACE::Run_Test;

$testoutputfilename = "test.log";
$status = 0;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");

PerlACE::add_lib_path('../FooType');

$DCPSREPO = new PerlACE::Process ("../../../dds/InfoRepo/DCPSInfoRepo",
                            "-o $dcpsrepo_ior"
                            . " -d $domains_file -ORBDebugLevel 1");


$Test = new PerlACE::Process ("infrastructure_test",
                               "-DCPSInfo file://$dcpsrepo_ior");

# save output to a faile because the output contaings "ERROR"
open(SAVEERR, ">&STDERR");
open(STDERR, ">$testoutputfilename") || die "ERROR: Can't redirect stderr";

$DCPSREPO->Spawn ();
sleep 5;

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

exit $status;
