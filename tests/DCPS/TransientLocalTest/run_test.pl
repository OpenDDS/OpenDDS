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
my $debug  = 0; #10;

my $opts = $debug ? "-DCPSDebugLevel $debug " : '';
my $cfg = ($ARGV[0] eq 'rtps') ? 'rtps.ini' : 'tcp.ini';
my $pub_opts = "$opts -DCPSConfigFile $cfg";
my $sub_opts = $pub_opts;

my $dcpsrepo_ior = "repo.ior";
my $repo_bit_opt = $opts;

unlink $dcpsrepo_ior;

my $data_file = "test_run.data";
unlink $data_file;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "$repo_bit_opt -o $dcpsrepo_ior ");
my $Subscriber = PerlDDS::create_process("subscriber", "$sub_opts");
my $Publisher = PerlDDS::create_process("publisher", "$pub_opts " .
                                        "-ORBLogFile $data_file");

print $DCPSREPO->CommandLine() . "\n";
print $Publisher->CommandLine() . "\n";

$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
}

$Publisher->Spawn();

if (PerlACE::waitforfile_timed($data_file, 30) == -1) {
    print STDERR "ERROR: waiting for Publisher file\n";
    $Publisher->Kill();
    $DCPSREPO->Kill();
    exit 1;
}

if (PerlACE::waitforfileoutput_timed($data_file, "Done writing", 90) == -1) {
    print STDERR "ERROR: waiting for Publisher output.\n";
    $Publisher->Kill();
    $DCPSREPO->Kill();
    exit 1;
}

#Sleep for 2 seconds after publisher send all samples to avoid the timing issue
#that the subscriber may start and finish in 1 second while the publisher is waiting
#for it to start.
sleep(2);

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();

my $PublisherResult = $Publisher->WaitKill(300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill(15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
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
