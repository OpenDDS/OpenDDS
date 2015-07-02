eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

$dcpsrepo_ior = "repo.ior";


unlink $dcpsrepo_ior;

# -b
$parameters = "-DcpsBit 0";
# or could have
# $parameters = "-b -DcpsBit 1";

if ($ARGV[0] eq 'by_instance') {
  $parameters .= " -i";
}

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "-o $dcpsrepo_ior"
                                      . " -NOBITS");

$ZCTest = PerlDDS::create_process ("main", $parameters);

print $DCPSREPO->CommandLine(), "\n";
if ($DCPSREPO->Spawn () != 0) {
    print STDERR "ERROR: Couldn't spawn InfoRepo\ntest FAILED.\n";
    return 1;
}

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $ZCTest->CommandLine(), "\n";
if ($ZCTest->Spawn () != 0) {
    print STDERR "ERROR: Couldn't spawn main\ntest FAILED.\n";
    return 1;
}

$result = $ZCTest->WaitKill (60);

if ($result != 0) {
    print STDERR "ERROR: main returned $result \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}
exit $status;
