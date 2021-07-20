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

my $discovery = "inforepo";
my $transport = "tcp";

my $pub_extra = "";

my $test = new PerlDDS::TestFramework();

while ($a = shift) {
    if ($a eq 'lost') {
        $pub_extra = "-l -n 4 -t 10 -c 8 -N 4 -C 16";
    }
    elsif ($a eq 'rtps') {
        $transport = "rtps";
    }
    elsif ($a eq 'rtpsdisco') {
        $discovery = "rtps";
        $test->{'discovery'} = 'rtps';
    }
    else {
        print STDERR "ERROR: unknown option: $a\n";
        exit 1;
    }
}

my $pub_conf = "pub_" . $discovery . "_" . $transport . ".ini";

my $pub_opts = "-DCPSConfigFile $pub_conf $pub_extra -ORBVerboseLogging 1";

$test->enable_console_logging();
$test->process('pub', 'publisher', $pub_opts);

$test->setup_discovery();

$test->start_process('pub');

exit $test->finish(60);
