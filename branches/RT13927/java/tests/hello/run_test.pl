eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env qw(ACE_ROOT TAO_ROOT DDS_ROOT);
use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Process_Java;
use strict;

my $status = 0;
my $debug = '0';

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
}

my $iorfile = 'server.ior';
unlink $iorfile;

my $SV = PerlDDS::create_process ("$TAO_ROOT/tests/Hello/server",
                               "-ORBDebugLevel $debug -o $iorfile");

PerlACE::add_lib_path ('.');
my $CL = new PerlDDS::Process_Java ('Client', "-k file://$iorfile -ORBDebugLevel $debug",
                          ['hello_java_client.jar']);

my $server_status = $SV->Spawn ();

if ($server_status != 0) {
    print STDERR "ERROR: server returned $server_status\n";
    exit 1;
}

if (PerlACE::waitforfile_timed ($iorfile,
        $PerlACE::wait_interval_for_process_creation) == -1) {
    print STDERR "ERROR: cannot find file <$iorfile>\n";
    $SV->Kill (); $SV->TimedWait (1);
    exit 1;
}

my $client_status = $CL->SpawnWaitKill (300);

if ($client_status != 0) {
    print STDERR "ERROR: client returned $client_status\n";
    $status = 1;
}

$server_status = $SV->WaitKill ($PerlACE::wait_interval_for_process_creation);

if ($server_status != 0) {
    print STDERR "ERROR: server returned $server_status\n";
    $status = 1;
}

unlink $iorfile;
exit $status;
