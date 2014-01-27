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

my $status = 0;
my $debug = '0';

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
}

my $opts = "-DCPSBit 0";
my $debug_opt = ($debug eq '0') ? ''
    : "-ORBDebugLevel $debug -DCPSDebugLevel $debug";
my $pub_opts = "$opts $debug_opt -ORBLogFile pub.log";

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process ("$DDS_ROOT/bin/DCPSInfoRepo", "-NOBITS".
            " -ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log -o $dcpsrepo_ior");

PerlACE::add_lib_path ("$DDS_ROOT/java/tests/messenger/messenger_idl");

my $PUB = new PerlDDS::Process_Java ('Both', $pub_opts,
                           ["$DDS_ROOT/java/tests/messenger/messenger_idl/".
                            "messenger_idl_test.jar"]);

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$PUB->Spawn ();

my $PublisherResult = $PUB->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: participant returned $PublisherResult \n";
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
