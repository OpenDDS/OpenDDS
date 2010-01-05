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

$opts =  "-ORBSvcConf ./tcp.conf";
$pub_opts = "$opts -DCPSConfigFile pub.ini -DCPSBit 0";
$sub_opts = "$opts -DCPSConfigFile sub.ini -DCPSBit 0";

if ($ARGV[0] eq 'udp') {
    $opts =  "-ORBSvcConf udp.conf -t udp -ORBSvcConf tcp.conf";
    $pub_opts = "$opts -DCPSConfigFile pub_udp.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'multicast') {
    $opts =  "-ORBSvcConf multicast.conf -ORBSvcConf tcp.conf";
    $pub_opts = "$opts -DCPSConfigFile pub_multicast.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_multicast.ini";
}
elsif ($ARGV[0] eq 'default_tcp') {
    $opts =  "-ORBSvcConf tcp.conf -t default_tcp";
    $pub_opts = "$opts";
    $sub_opts = "$opts";
}
elsif ($ARGV[0] eq 'default_udp') {
    $opts =  "-ORBSvcConf udp.conf -ORBSvcConf tcp.conf"
        . " -t default_udp";
    $pub_opts = "$opts";
    $sub_opts = "$opts";
}
elsif ($ARGV[0] eq '-n') {
    $pub_opts = $pub_opts." -n $ARGV[1]";
}
elsif ($ARGV[0] eq 'default_multicast') {
    $opts =  "-ORBSvcConf multicast.conf -ORBSvcConf tcp.conf";
    $pub_opts = "$opts -t default_multicast";
    $sub_opts = "$opts -t default_multicast";
}
elsif ($ARGV[0] eq '-debug') {
    if($ARGV[1] eq '-n') {
      $pub_opts = $pub_opts." -d -n $ARGV[2]";
    }
    else {
      $pub_opts = $pub_opts." -d";
    }
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

$dcpsrepo_ior = "repo.ior";
$repo_bit_opt = "-NOBITS";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "$repo_bit_opt -o $dcpsrepo_ior ");
$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
$Publisher = PerlDDS::create_process ("publisher", " $pub_opts");

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

$SubscriberResult = $Subscriber->WaitKill (15);
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
