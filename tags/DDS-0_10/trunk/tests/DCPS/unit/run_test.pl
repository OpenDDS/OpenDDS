eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;
my $debug = 0;

$testoutputfilename = "test.log";

$iorfile = PerlACE::LocalFile ("repo.ior");
unlink $iorfile;
$client_orb = 0;

if ($ARGV[0] eq "-client_orb") {
 $client_orb = 1;
}

$REPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo"
			      , "-o $iorfile -d domain_ids -NOBITS");

$svc_config=" -ORBSvcConf ../../tcp.conf ";
$CL = new PerlACE::Process ("DdsDcps_UnitTest"
			    , " -DCPSInfoRepo file://$iorfile $svc_config -c $client_orb");

print $REPO->CommandLine() . "\n" if $debug ;
$REPO->Spawn ();

if (PerlACE::waitforfile_timed ($iorfile, 5) == -1) {
    print STDERR "ERROR: cannot find file <$iorfile>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
    exit 1;
}

print $CL->CommandLine() . "\n" if $debug ;

# save output to a faile because the output contaings "ERROR"
open(SAVEERR, ">&STDERR");
open(STDERR, ">$testoutputfilename") || die "ERROR: Can't redirect stderr";

$client = $CL->SpawnWaitKill (30);

close(STDERR);
open(STDERR, ">&SAVEERR") || die "ERROR: Can't redirect stderr";
close (SAVEERR);

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
