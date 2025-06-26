eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Path;
use strict;

my $test = new PerlDDS::TestFramework();
$test->{'nobits'} = 1;

$test->process('sub', 'subscriber');
$test->process('pub', 'publisher');

$test->setup_discovery();

rmtree('./DCS');

$test->start_process('pub');
$test->start_process('sub');

exit $test->finish(60);
