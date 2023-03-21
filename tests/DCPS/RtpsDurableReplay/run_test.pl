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

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();
$test->{'discovery'} = 'rtps';

$test->process('publisher', 'publisher', '');
$test->process('subscriber', 'subscriber', '');

$test->setup_discovery();

rmtree './DCS';

$test->start_process('publisher');
$test->start_process('subscriber');

exit $test->finish(60);
