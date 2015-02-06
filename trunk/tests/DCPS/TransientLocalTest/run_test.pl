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
use strict;

my $status = 0;
my $debug  = 0;

my $opts = $debug ? "-DCPSDebugLevel $debug -DCPSTransportDebugLevel $debug ".
                    "-ORBVerboseLogging 1 " : '';
my $cfg = ($ARGV[0] eq 'rtps') ? 'rtps.ini' : 'tcp.ini';
my $pub_opts = $opts . "-DCPSConfigFile $cfg -DCPSBit 0";
my $sub_opts = $pub_opts;

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "$opts -o $dcpsrepo_ior -NOBITS");
my $Subscriber1 = PerlDDS::create_process("subscriber", $sub_opts);
my $Subscriber2 = PerlDDS::create_process("subscriber", $sub_opts);
my $Publisher = PerlDDS::create_process("publisher", $pub_opts);

print $DCPSREPO->CommandLine() . "\n";

$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
}

print $Subscriber1->CommandLine() . "\n";
$Subscriber1->Spawn();

sleep(2);

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

# Sleep for 2 seconds for publisher to write durable data.
sleep(2);

print $Subscriber2->CommandLine() . "\n";
$Subscriber2->Spawn();

my $PublisherResult = $Publisher->WaitKill(300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $Subscriber1Result = $Subscriber1->WaitKill(15);
if ($Subscriber1Result != 0) {
    print STDERR "ERROR: subscriber1 returned $Subscriber1Result \n";
    $status = 1;
}
my $Subscriber2Result = $Subscriber2->WaitKill(15);
if ($Subscriber2Result != 0) {
    print STDERR "ERROR: subscriber2 returned $Subscriber2Result \n";
    $status = 2;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
