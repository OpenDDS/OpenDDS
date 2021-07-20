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

my $debug = 0;
my $opts = $debug ? "-DCPSDebugLevel $debug " : '';

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();
$test->{'add_transport_config'} = 0;

if ($test->{'flags'}->{'rtps'}) {
    $opts .= 'rtps';
}

$test->process("findtopic", "findtopic", $opts);
$test->start_process("findtopic");

my $client = $test->finish(15);
my $status = 0;
if ($client != 0) {
    print STDERR "ERROR: client returned $client\n";
    $status = 1;
}
exit $status;
