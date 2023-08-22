eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use warnings;
use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use POSIX qw(floor);
use strict;

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

my $debug ;# = 10;
my $repoDebug;
my $pubDebug;
my $debugFile;
$repoDebug = $debug if not $repoDebug and $debug;
$pubDebug  = $debug if not $pubDebug  and $debug;

my $transportDebug;
my $repoTransportDebug;
my $pubTransportDebug;
$repoTransportDebug = $transportDebug if not $repoTransportDebug and $transportDebug;
$pubTransportDebug  = $transportDebug if not $pubTransportDebug  and $transportDebug;

my $test = new PerlDDS::TestFramework();
$test->{'wait_after_first_proc'} = 50;
$test->enable_console_logging();

my $repoArgs = "";
$repoArgs .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoArgs .= "-DCPSTransportDebugLevel $repoTransportDebug " if $repoTransportDebug;
$repoArgs .= "-ORBLogFile $debugFile " if $repoDebug and $debugFile;
$test->setup_discovery($repoArgs);

my $args = "";
$args .= "-DCPSDebugLevel $pubDebug " if $pubDebug;
$args .= "-DCPSTransportDebugLevel $pubTransportDebug " if $pubTransportDebug;
$args .= "-ORBLogFile $debugFile " if $pubDebug and $debugFile;

$test->process("WaitSetLivelinessLost", 'WaitSetLivelinessLost', $args);
$test->start_process('WaitSetLivelinessLost');

my $result = $test->finish(60, "WaitSetLivelinessLost");

exit $result;
