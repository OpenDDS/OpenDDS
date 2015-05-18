eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path("Idl");

my $test = new PerlDDS::TestFramework();

$test->enable_console_logging();

$test->process('Subscriber1', 'Subscriber/subscriber', '1');
$test->process('Subscriber2', 'Subscriber/subscriber', '2');
$test->process('Subscriber3', 'Subscriber/subscriber', '3');
$test->start_process('Subscriber1');
$test->start_process('Subscriber2');
$test->start_process('Subscriber3');

sleep 3;

$test->process('Publisher1', 'Publisher/publisher', '1');
$test->process('Publisher2', 'Publisher/publisher', '2');
$test->start_process('Publisher1', );
$test->start_process('Publisher2', );
exit $test->finish(30);
