eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

my $status = 0;
my $options = "-DCPSBit 0";

my $test = new PerlDDS::TestFramework();
$test->process("unregister_type_test", "unregister_type_test", "$options");
$test->setup_discovery("-NOBITS");
$test->start_process("unregister_type_test");
$result = $test->finish(60);

if ($result != 0) {
    print STDERR "ERROR: test returned $result\n";
    $status = 1;
}

exit $status;
