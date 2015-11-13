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

my $test = new PerlDDS::TestFramework();
$test->process ("TestFixed", "TestFixed", "");
$test->start_process ("TestFixed");

$status = $test->finish(300);

if ($status != 0) {
    print STDERR "ERROR: TestFixed returned $status\n";
    $status = 1;
}

exit $status;
