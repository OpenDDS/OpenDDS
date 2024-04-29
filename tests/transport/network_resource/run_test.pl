eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;
my $status = 0;

my $test = new PerlDDS::TestFramework();
$test->process("nr", "NetworkResource", "");
$test->start_process("nr");
my $retcode = $test->finish(20);
if ($retcode != 0) {
  $status = 1;
}

exit $status;
