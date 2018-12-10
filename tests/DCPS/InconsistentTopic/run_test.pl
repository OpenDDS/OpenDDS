eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();

$test->process('sub', 'subscriber');
$test->process('pub', 'publisher');

$test->setup_discovery();

$test->start_process('pub');
$test->start_process('sub');

exit $test->finish(300);
