eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path("../Messenger/Idl");

my $test = new PerlDDS::TestFramework();

$test->enable_console_logging();

$test->process('Subscriber', 'Subscriber/subscriber');
$test->start_process('Subscriber');
sleep 5;

$test->process('Publisher', 'Publisher/publisher');
$test->start_process('Publisher');
exit $test->finish(120);
