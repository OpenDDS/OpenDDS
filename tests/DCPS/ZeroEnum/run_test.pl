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
$test->process('publisher', 'publisher', '-DCPSDebugLevel 4 -DCPSConfigFile rtps.ini ');
$test->process('subscriber', 'subscriber', '-DCPSDebugLevel 4 -DCPSConfigFile rtps.ini ');

$test->start_process('publisher');
$test->start_process('subscriber');

exit $test->finish(30);
