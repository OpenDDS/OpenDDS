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

$parameters = "-DcpsBit 0 -ORBVerboseLogging 1 -DCPSDebugLevel 10";

if ($ARGV[0] eq 'by_instance') {
  $parameters .= " -i";
}

my $test = new PerlDDS::TestFramework();
$test->process("WriteDataContainerTest", "WriteDataContainerTest", $parameters);
$test->start_process("WriteDataContainerTest");
$result = $test->finish(60);
if ($result != 0) {
    print STDERR "ERROR: main returned $result \n";
    $status = 1;
}
exit $status;
