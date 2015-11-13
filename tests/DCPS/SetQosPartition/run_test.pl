eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

my $debug = 10;
my $transportDebug = 10;
my $debugFile = "debug.out";
my $is_rtps_disc = 0;
my $DCPScfg = "";

if ($ARGV[0] eq "rtps_disc") {
  $DCPScfg = $ARGV[0] . ".ini";
  $is_rtps_disc = 1;
} elsif ($ARGV[0] eq "rtps_disc_tcp") {
  $DCPScfg = $ARGV[0] . ".ini";
  $is_rtps_disc = 1;
}

my $debugOpts = "";

$debugOpts .= "-DCPSDebugLevel $debug " if $debug;
$debugOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$debugOpts .= "-ORBLogFile $debugFile " if $debugFile and ($debug or $transportDebug);

my $opts;

$opts .= " " . $debugOpts if $debug or $transportDebug;

$pub_opts = "$opts -DCPSConfigFile " .  ($is_rtps_disc ? $DCPScfg : "pub.ini");
$sub_opts = "$opts -DCPSConfigFile " .  ($is_rtps_disc ? $DCPScfg : "sub.ini");

$repo_bit_opt = $opts;

unlink $debugFile;

my $test = new PerlDDS::TestFramework();
$test->setup_discovery("$repo_bit_opt") unless $is_rtps_disc;
$test->process("subscriber", "subscriber", "$sub_opts");
$test->process("publisher", "publisher", "$pub_opts");
$test->start_process("publisher");
$test->start_process("subscriber");

my $result = $test->finish(300);
if ($result != 0) {
    print STDERR "ERROR: test returned $result\n";
    $status = 1;
}

exit $status;
