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
my $debugFile;
my $debug;
#$debug = 10;
$debugFile = "test.log";
my $debugOpts = "";
$debugOpts .= "-DCPSDebugLevel $debug " if $debug;

unlink $debugFile;

my $test = new PerlDDS::TestFramework();
$test->process("dpshutdown", "dpshutdown", "$debugOpts $pub_opts");
$test->start_process("dpshutdown");
$client = $test->finish(30);

if ($client != 0) {
    print STDERR "ERROR: client returned $client\n";
    $status = 1;
}

exit $status;
