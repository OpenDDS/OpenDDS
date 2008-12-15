eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

$status = 0;
$use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');

$opts = $use_svc_config ? "-ORBSvcConf tcp.conf" : '';
$repo_bit_opt = $opts;

$pub_opts = "$opts -ORBDebugLevel 10 -ORBLogFile pub.log -DCPSConfigFile pub.ini -DCPSDebugLevel 10";
$sub_opts = "$opts -ORBDebugLevel 10 -ORBLogFile sub.log -DCPSConfigFile sub.ini -DCPSDebugLevel 10";

if ($ARGV[0] eq 'udp') {
    $opts .= ($use_svc_config ? " -ORBSvcConf udp.conf " : '') . "-t udp";
    $pub_opts = "$opts -DCPSConfigFile pub_udp.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'mcast') {
    $opts .= ($use_svc_config ? " -ORBSvcConf mcast.conf " : '') . "-t mcast";
    $pub_opts = "$opts -DCPSConfigFile pub_mcast.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_mcast.ini";
}
elsif ($ARGV[0] eq 'reliable_mcast') {
    $opts .= ($use_svc_config ? " -ORBSvcConf reliable_mcast.conf " : '')
        . "-t reliable_mcast";
    $pub_opts = "$opts -DCPSConfigFile pub_reliable_mcast.ini -DCPSTransportDebugLevel 3";
    $sub_opts = "$opts -DCPSConfigFile sub_reliable_mcast.ini -DCPSTransportDebugLevel 3";
}
elsif ($ARGV[0] eq 'default_tcp') {
    $opts .= " -t default_tcp";
    $pub_opts = "$opts";
    $sub_opts = "$opts";
}
elsif ($ARGV[0] eq 'default_udp') {
    $opts .= ($use_svc_config ? " -ORBSvcConf udp.conf " : '')
	. " -t default_udp";
    $pub_opts = "$opts";
    $sub_opts = "$opts";
}
elsif ($ARGV[0] eq 'default_mcast') {
    $opts .= ($use_svc_config ? " -ORBSvcConf mcast.conf " : '');
    $pub_opts = "$opts -t default_mcast_pub";
    $sub_opts = "$opts -t default_mcast_sub";
}
elsif ($ARGV[0] eq 'default_reliable_mcast') {
    $opts .= ($use_svc_config ? " -ORBSvcConf reliable_mcast.conf " : '');
    $pub_opts = "$opts -t default_reliable_mcast_pub";
    $sub_opts = "$opts -t default_reliable_mcast_sub";
}
elsif ($ARGV[0] eq 'nobits') {
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= ' -DCPSBit 0';
    $sub_opts .= ' -DCPSBit 0';
}
elsif ($ARGV[0] eq 'ipv6') {
    $pub_opts = "$opts -DCPSConfigFile pub_ipv6.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_ipv6.ini";
}
elsif ($ARGV[0] eq 'stack') {
    $opts .= " -t default_tcp";
    $pub_opts = "$opts";
    $sub_opts = "$opts";
    $stack_based = 1;
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
				  "-ORBDebugLevel 10 -ORBLogFile DCPSInfoRepo.log $repo_bit_opt -o $dcpsrepo_ior ");

if($stack_based == 0) {
  #create
  $Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
}
else {
  $Subscriber = PerlDDS::create_process ("stack_subscriber", " $sub_opts");
}

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
