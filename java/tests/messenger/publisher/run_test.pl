eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env qw(ACE_ROOT DDS_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Process_Java;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $status = 0;
my $debug = '0';

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
}

my $config = pop;
if ($config eq '') {
    $config = 'tcp';
}

my $use_repo = ($config !~ /^rtps_disc/);

my $reliable = '-r';

if ($config eq 'udp') {
  $reliable = '';
}

my $opts = "-DCPSBit 0 -DCPSConfigFile ../$config.ini $reliable";
my $pub_opts = $opts;
my $sub_opts = $opts;
if ($debug ne '0') {
    my $debug_opt = "-ORBDebugLevel $debug -DCPSDebugLevel $debug " .
                    "-DCPSTransportDebugLevel $debug";
    $pub_opts .= " $debug_opt -ORBLogFile pub.log";
    $sub_opts .= " $debug_opt -ORBLogFile sub.log";
}

my $dcpsrepo_ior = 'repo.ior';

unlink $dcpsrepo_ior;
unlink qw/pub.log sub.log DCPSInfoRepo.log/;

my $DCPSREPO = PerlDDS::create_process("$DDS_ROOT/bin/DCPSInfoRepo",
                 '-NOBITS' . (($debug eq '0' ? '' : " -ORBDebugLevel $debug" .
                 ' -ORBLogFile DCPSInfoRepo.log')) . " -o $dcpsrepo_ior");

my $SUB = PerlDDS::create_process("$DDS_ROOT/tests/DCPS/Messenger/subscriber",
                                  $sub_opts);

PerlACE::add_lib_path("$DDS_ROOT/java/tests/messenger/messenger_idl");
PerlACE::add_lib_path("$DDS_ROOT/tests/DCPS/Messenger");

my $PUB = new PerlDDS::Process_Java('TestPublisher', $pub_opts,
            ["$DDS_ROOT/java/tests/messenger/messenger_idl/".
             'messenger_idl_test.jar']);

if ($use_repo) {
    $DCPSREPO->Spawn();
    if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
        print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
        $DCPSREPO->Kill();
        exit 1;
    }
}

$PUB->Spawn();

$SUB->Spawn();

my $PublisherResult = $PUB->WaitKill(300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult\n";
    $status = 1;
}

my $SubscriberResult = $SUB->WaitKill(30);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
    $status = 1;
}

if ($use_repo) {
    my $ir = $DCPSREPO->TerminateWaitKill(5);
    if ($ir != 0) {
        print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
        $status = 1;
    }
    unlink $dcpsrepo_ior;
}

if ($status == 0) {
    print "test PASSED.\n";
} else {
    print STDERR "test FAILED.\n";
}

exit $status;
