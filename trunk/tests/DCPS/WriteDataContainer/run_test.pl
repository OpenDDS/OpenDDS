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


$parameters = "-DcpsBit 0 -ORBVerboseLogging 1 -DCPSDebugLevel 10";

if ($ARGV[0] eq 'by_instance') {
  $parameters .= " -i";
}


$ZCTest = PerlDDS::create_process ("WriteDataContainerTest", $parameters);

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

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}
exit $status;
