eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Path;
use strict;

my $test = new PerlDDS::TestFramework();

$test->process('old', 'OldDevice/OldDevice');
$test->process('new', 'NewDevice/NewDevice');
$test->process('ctrl', 'Controller/Controller');

rmtree('DCS');

$test->start_process('old');
$test->start_process('new');
$test->start_process('ctrl');

exit($test->finish(60));
