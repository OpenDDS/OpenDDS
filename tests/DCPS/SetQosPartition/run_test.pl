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

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();
$test->{'dcps_debug_level'} = 4; # $test->{'dcps_transport_debug_level'} = 10;
$test->setup_discovery();
$test->process("pubsub", "pubsub", "-DCPSPendingTimeout 0");
$test->start_process("pubsub");
exit $test->finish(120);
