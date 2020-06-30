eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;
use warnings;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $result = 0;
my $dcps_debug_lvl = 1;

sub runTest {
    my $delay = shift;

    my $test = new PerlDDS::TestFramework();
    $test->enable_console_logging();

    print "*********************************\n";
    print "DomainRangeTest creates a single process with DWs and DRs\n";
    print "in several domains. Domains and transports are dynamically\n";
    print "configured from the templates in config.ini\n";
    print "The DW in each domain sends 10 messages to its DR.\n";
    print "*********************************\n";

    my $dw_static = 1;
    my $dr_static = 1;
    my $origin = 1;

    $test->process("alpha", 'DomainRangeTest', "-DCPSConfigFile config.ini -DCPSDebugLevel $dcps_debug_lvl -domain 2 -domain 15 -domain 4 -domain 7");
    $test->start_process("alpha");
    sleep $delay;

    my $res = $test->finish(150);
    if ($res != 0) {
        print STDERR "ERROR: test returned $res\n";
        $result += $res;
    }
}

runTest(0);

exit $result;
