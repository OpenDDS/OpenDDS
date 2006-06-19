eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;
my $debug = 0 ;

$domains_file = PerlACE::LocalFile ("domainids.txt");
$dcpsrepo_ior = PerlACE::LocalFile ("dcps_ir.ior");

unlink $dcpsrepo_ior;


$DCPSREPO = new PerlACE::Process ("../../../dds/InfoRepo/DCPSInfoRepo",
                            "-o $dcpsrepo_ior"
                            . " -d $domains_file -NOBITS");


$PUBLISHER = new PerlACE::Process ("publisher",
                            "-k file://$dcpsrepo_ior -q");

$SUBSCRIBER = new PerlACE::Process ("subscriber",
                            "-k file://$dcpsrepo_ior");

print $DCPSREPO->CommandLine() . "\n" if $debug ;
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 5) == -1) {
    print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
    exit 1;
} 

print $PUBLISHER->CommandLine() . "\n" if $debug ;
$PUBLISHER->Spawn ();
sleep 2;

print $SUBSCRIBER->CommandLine() . "\n" if $debug ;
$SUBSCRIBER->Spawn ();


$PubResult = $PUBLISHER->WaitKill (35);

if ($PubResult != 0) {
    print STDERR "ERROR: publisher returned $PubResult\n";
    $status = 1;
}

$SubResult = $SUBSCRIBER->WaitKill (35);

if ($SubResult != 0) {
    print STDERR "ERROR: subscriber returned $SubResult\n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(30);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

exit $status;
