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

$opts = '';
$repo_bit_opt = $opts;

$pub_opts = "$opts -ORBDebugLevel 10 -ORBLogFile pub.log -DCPSConfigFile pubsub.ini -DCPSDebugLevel 10";
$sub_opts = "$opts -DCPSTransportDebugLevel 6 -ORBDebugLevel 10 -ORBLogFile sub.log -DCPSConfigFile pubsub.ini -DCPSDebugLevel 10";

if ($ARGV[0] eq 'udp') {
    shift @ARGV;
    $opts .= "-t udp";
    $pub_opts = "$opts -DCPSConfigFile pub_udp.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'multicast') {
    shift @ARGV;
    $opts .= "-t multicast";
    $pub_opts = "$opts -DCPSConfigFile pubsub_multicast.ini";
    $sub_opts = "$opts -DCPSConfigFile pubsub_multicast.ini";
}
elsif ($ARGV[0] eq 'nobits') {
    shift @ARGV;
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= ' -DCPSBit 0';
    $sub_opts .= ' -DCPSBit 0';
}
elsif ($ARGV[0] eq 'ipv6') {
    shift @ARGV;
    $pub_opts = "$opts -DCPSConfigFile pubsub_ipv6.ini";
    $sub_opts = "$opts -DCPSConfigFile pubsub_ipv6.ini";
}
elsif ($ARGV[0] eq 'tcp') {
    shift @ARGV;
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

$argv_opts = join " ", @ARGV;
print "$argv_opts\n";

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

$Subscriber = PerlDDS::create_process ("pubsub", " $sub_opts $argv_opts");
$Publisher = PerlDDS::create_process ("pubsub", " $pub_opts $argv_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


$PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (100);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
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
