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

if ($test->flag('freeindex')) {
  $test->process('Test', 'FreeIndexTest');
} elsif ($test->flag('memorypool')) {
  $test->process('Test', 'MemoryPoolTest');
} elsif ($test->flag('safetyprofilepool')) {
  $test->process('Test', 'SafetyProfilePoolTest');
} elsif ($test->flag('qossettings')) {
  $test->process('Test', 'QosSettingsTest');
} elsif ($test->flag('sequences')) {
  $test->process('Test', 'SequencesTest');
}

$test->start_process('Test');

sleep 5;

exit $test->finish(30);
