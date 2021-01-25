eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

$status = 0;
my $debugFile;
my $debug;
#$debug = 10;
$debugFile = "test.log";
my $debugOpts = "";
$debugOpts .= "-DCPSDebugLevel $debug " if $debug;

unlink $debugFile;

my $test = new PerlDDS::TestFramework();
$test->{add_transport_config} = 0;

if ($test->flag('rtps_udp')) {
    $pub_opts .= " -t rtps_udp";
}
elsif ($test->flag('tcp')) {
    $pub_opts .= " -t tcp";
}
elsif ($test->flag('shmem')) {
    $pub_opts .= " -t shmem";
}
elsif ($test->flag('multicast')) {
    $pub_opts .= " -t multicast";
}
elsif ($test->flag('udp')) {
    $pub_opts .= " -t udp";
}

$test->process("containsentity", "containsentity", "$debugOpts $pub_opts");
$test->start_process("containsentity");
$client = $test->finish(30);

if ($client != 0) {
    print STDERR "ERROR: client returned $client\n";
    $status = 1;
}

exit $status;
