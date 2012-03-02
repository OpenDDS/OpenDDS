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

$status = 0;
my $debugFile;
my $debug;
# $debug = 10;
$debugFile = "test.log";
my $debugOpts = "";
$debugOpts .= "-DCPSDebugLevel $debug " if $debug;
$debugOpts .= "-ORBLogFile $debugFile ";

unlink $debugFile;

$iorfile = "repo.ior";
unlink $iorfile;
$client_orb = 0;

if ($ARGV[0] eq "-client_orb") {
 $client_orb = 1;
}


$REPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo"
                              , "$debugOpts -o $iorfile");

$CL = PerlDDS::create_process ("DdsDcps_UnitTest",
                              "$debugOpts -DCPSInfoRepo file://$iorfile " .
                              "-c $client_orb ");

print $REPO->CommandLine() . "\n";
$REPO->Spawn ();

if (PerlACE::waitforfile_timed ($iorfile, 30) == -1) {
    print STDERR "ERROR: cannot find file <$iorfile>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
    exit 1;
}

print $CL->CommandLine() . "\n";

$client = $CL->SpawnWaitKill (30);

if ($client != 0) {
    print STDERR "ERROR: client returned $client\n";
    $status = 1;
}

$REPO->TerminateWaitKill (5);

unlink $iorfile;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
