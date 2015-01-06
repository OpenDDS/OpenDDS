eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $debug ;# = 10;
my $repoDebug;
my $subDebug;
my $pubDebug;
my $debugFile;
$repoDebug = $debug if not $repoDebug and $debug;
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
my $cfg = ($ARGV[0] eq 'rtps') ? 'rtps.ini' : 'tcp.ini';
my $pub_opts = $opts . "-DCPSConfigFile $cfg -DCPSBit 0";
my $sub_opts = $pub_opts;

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $Subscriber2 = PerlDDS::create_process("subscriber", $sub_opts);

my $repoArgs;

$repoArgs .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoArgs .= "-DCPSTransportDebugLevel $repoTransportDebug " if $repoTransportDebug;
$repoArgs .= "-ORBLogFile $debugFile "     if $repoDebug and $debugFile;

my $test = new PerlDDS::TestFramework();
$test->{'wait_after_first_proc'} = 50;
$test->enable_console_logging();

my $repoArgs = "";
$test->setup_discovery($repoArgs);

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
