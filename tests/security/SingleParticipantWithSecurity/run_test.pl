eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../../DCPS/ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();

$test->{dcps_debug_level} = 4;
$test->{dcps_transport_debug_level} = 2;
$test->{add_transport_config} = 0;

my $pubsub_opts .= "-ORBDebugLevel 1 -DCPSConfigFile rtps_disc.ini";

$test->process("SingleParticipantWithSecurity", "SingleParticipantWithSecurity", $pubsub_opts);
$test->start_process("SingleParticipantWithSecurity");

# start killing processes in 30 seconds
exit $test->finish(30);
