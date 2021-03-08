eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();


$test->enable_console_logging();

if ($test->flag('safetyprofilepool')) {
  $test->process('Test', 'SafetyProfilePoolTest');
}

$test->start_process('Test');

sleep 5;

exit $test->finish(30);
