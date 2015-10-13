eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;
use warnings;

my $result = 0;
my $reliable = 1;
my $dcps_debug_lvl = 0;

sub runTest {
    my $delay = shift;

    my $test = new PerlDDS::TestFramework();
    $test->enable_console_logging();
    $test->setup_discovery();

    if ($test->flag('best_effort')) {
        $reliable = 0;
    }

    print "*********************************\n";
    print "MultiDiscoveryTest creates 3 processes, each with a DW and DR.\nDW's in each process use different forms of discovery to find/associate with the DR in their respective domain.\n";
    print "One DW is specified as the 'origin' and sends 10 messages to its associated DR.\nUpon receipt, DR's pass the message to their process's DW which add's its id to the message's from field\nand relays the messgae on until the loop is completed at the 'origin' DR\n";
    print "*********************************\n";
    print "Spawning alpha - Writer (12) in domain 12 using default discovery and Reader (13) in domain 31 using rtps discovery\n";
    my $dw_static = 1;
    my $dr_static = 1;
    my $origin = 1;

    $test->process("alpha", 'MultiDiscoveryTest', "-DCPSConfigFile config.ini -DCPSDebugLevel $dcps_debug_lvl -origin $origin -reliable $reliable -dw_static_disc 0 -dr_static_disc 0 -wdomain 12 -rdomain 31 -writer 000012 -reader 000013");
    $test->start_process("alpha");
    sleep $delay;

    print "Spawning beta - Writer (23) in domain 23 using static discovery and Reader (21) in domain 12 using default discovery\n";
    $origin = 0;
    $test->process("beta", 'MultiDiscoveryTest', "-DCPSConfigFile config.ini -DCPSDebugLevel $dcps_debug_lvl -origin $origin -reliable $reliable -dw_static_disc $dw_static -dr_static_disc 0 -wdomain 23 -rdomain 12 -dw_participant 000000000023 -writer 000023 -reader 000021");
    $test->start_process("beta");
    sleep $delay;

    print "Spawning gamma - Writer (31) in domain 31 using rtps discovery and Reader (32) in domain 23 using static discovery\n";
    $test->process("gamma", 'MultiDiscoveryTest', "-DCPSConfigFile config.ini -DCPSDebugLevel $dcps_debug_lvl -origin $origin -reliable $reliable -dw_static_disc 0 -dr_static_disc $dr_static -wdomain 31 -rdomain 23 -dr_participant 000000000032 -writer 000031 -reader 000032");
    $test->start_process("gamma");

    my $res = $test->finish(150);
    if ($res != 0) {
        print STDERR "ERROR: test returned $res\n";
        $result += $res;
    }
}

runTest(0);

exit $result;
