eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use PerlACE::Run_Test;
use strict;
use Env qw(JAVA_HOME DDS_ROOT TAO_ROOT);

my $status = 0;
my $debug = '0';

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    } 
}

my $iorfile = 'server.ior';
unlink $iorfile;

my $SV = new PerlACE::Process ("$TAO_ROOT/tests/Hello/server",
                               "-ORBDebugLevel $debug -o $iorfile");

PerlACE::add_lib_path ("$DDS_ROOT/lib");
PerlACE::add_lib_path ('.');
my @classpaths = ("$DDS_ROOT/lib/i2jrt.jar", "hello_java_client.jar");
my $sep = ':';
my $jnid;
if ($^O eq 'MSWin32') {
    $sep = ';';
    $jnid = '-Djni.nativeDebug=1'
        unless $PerlACE::Process::ExeSubDir =~ /Release/i;
}
my $classpath = join ($sep, @classpaths);
my $CL = new PerlACE::Process ("$JAVA_HOME/bin/java",
                               "-Xcheck:jni -cp $classpath $jnid Client " .
                               "-k file://$iorfile -ORBDebugLevel $debug");
$CL->IgnoreExeSubDir (1);

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
