eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $num_topics = 200;
my $num_samples = 10;

my $opts = "-w $num_topics -s $num_samples " .
           "-ORBDebugLevel 10 -DCPSDebugLevel 10 -DCPSTransportDebugLevel 6";
my $repo_bit_opt = $opts;

my $pub_opts = "$opts -ORBLogFile pub.log -DCPSConfigFile pubsub.ini ";
my $sub_opts = "$opts -ORBLogFile sub.log -DCPSConfigFile pubsub.ini ";

if ($ARGV[0] eq 'udp') {
    shift @ARGV;
    $pub_opts = "$opts -ORBLogFile pub.log -DCPSConfigFile pub_udp.ini";
    $sub_opts = "$opts -ORBLogFile sub.log -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'multicast') {
    shift @ARGV;
    $pub_opts = "$opts -ORBLogFile pub.log -DCPSConfigFile pubsub_multicast.ini";
    $sub_opts = "$opts -ORBLogFile sub.log -DCPSConfigFile pubsub_multicast.ini";
}
elsif ($ARGV[0] eq 'nobits') {
    shift @ARGV;
    $repo_bit_opt .= '-NOBITS';
    $pub_opts .= ' -DCPSBit 0';
    $sub_opts .= ' -DCPSBit 0';
}
elsif ($ARGV[0] eq 'ipv6') {
    shift @ARGV;
    $pub_opts = "$opts -ORBLogFile pub.log -DCPSConfigFile pubsub_ipv6.ini";
    $sub_opts = "$opts -ORBLogFile sub.log -DCPSConfigFile pubsub_ipv6.ini";
}
elsif ($ARGV[0] eq 'tcp') {
    shift @ARGV;
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

my $argv_opts = join " ", @ARGV;
print "$argv_opts\n";

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "-ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

my $Subscriber = PerlDDS::create_process ("pubsub", " $sub_opts $argv_opts");
my $Publisher = PerlDDS::create_process ("pubsub", " $pub_opts $argv_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();


my $PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill (100);
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
