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

my $debug ;# = 10;
my $subDebug;
my $pubDebug;
my $debugFile;
$subDebug  = $debug if not $subDebug  and $debug;
$pubDebug  = $debug if not $pubDebug  and $debug;

my $transportDebug;
my $repoTransportDebug;
my $subTransportDebug ;# = 10;
my $pubTransportDebug;
$repoTransportDebug = $transportDebug if not $repoTransportDebug and $transportDebug;
$subTransportDebug  = $transportDebug if not $subTransportDebug  and $transportDebug;
$pubTransportDebug  = $transportDebug if not $pubTransportDebug  and $transportDebug;


my $opts = $debug ? "-DCPSDebugLevel $debug -DCPSTransportDebugLevel $debug ".
                    "-ORBVerboseLogging 1 " : '';
my $cfg = ($ARGV[0] eq 'rtps') ? 'rtps.ini' : ($ARGV[0] eq 'rtps_disc') ? 'rtps_disc.ini' : 'tcp.ini';
my $pub_opts = $opts . "-DCPSConfigFile $cfg -DCPSBit 0";
my $sub_opts = $pub_opts;

my $test = new PerlDDS::TestFramework();
$test->{'wait_after_first_proc'} = 50;
$test->enable_console_logging();

$test->setup_discovery();

$test->process('pub', 'publisher', $pub_opts);
$test->process('sub1', 'subscriber', $sub_opts);
$test->process('sub2', 'subscriber', $sub_opts);

$test->start_process('sub1');

sleep(2);

$test->start_process('pub');

# Sleep for 2 seconds for publisher to write durable data.
sleep(2);

$test->start_process('sub2');

exit $test->finish(90, 'pub');
