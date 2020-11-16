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

my $status = 0;
my $debug = 0;
# $debug = 10;
my $debugOpts = "-DCPSDebugLevel $debug" if $debug;
my $pub_opts = "-DCPSConfigFile rtps_disc.ini";
my $test = new PerlDDS::TestFramework();
$test->process("findtopic", "findtopic", "$debugOpts $pub_opts");
$test->start_process("findtopic");
my $client = $test->finish(15);
if ($client != 0) {
    print STDERR "ERROR: client returned $client\n";
    $status = 1;
}
exit $status;
