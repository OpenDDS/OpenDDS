eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();
$test->setup_discovery();
$test->enable_console_logging();
$test->{dcps_debug_level} = 1;
$test->{transport} = 'rtps';
my $pending_timeout = '-DCPSPendingTimeout 2';
my $verbose = $test->flag('--verbose');
$test->process('sub', 'subscriber', ($verbose ? '-v' : '') . "$pending_timeout");
$test->process('pub', 'publisher', ($verbose ? '-v ' : '') .
                                   ($test->flag('--publisher') ? '-p ' : '') .
                                    "$pending_timeout");

$test->start_process('sub');
$test->start_process('pub');

exit $test->finish(300);
