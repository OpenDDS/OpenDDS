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

$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');

$opts = $use_svc_conf ? "-ORBSvcConf tcp.conf" : '';
$pub_opts = "$opts -DCPSConfigFile pub.ini";
$sub_opts = "$opts -DCPSConfigFile sub.ini";

if ($ARGV[0] eq 'udp') {
    $opts = ($use_svc_conf ? "-ORBSvcConf udp.conf" : '') . " -t udp";
    $pub_opts = "$opts -DCPSConfigFile pub_udp.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'mcast') {
    $opts = ($use_svc_conf ? "-ORBSvcConf mcast.conf" : '') . " -t mcast";
    $pub_opts = "$opts -DCPSConfigFile pub_mcast.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_mcast.ini";
}
elsif ($ARGV[0] eq 'default_tcp') {
    $opts = $use_svc_conf ? "-ORBSvcConf tcp.conf" : '';
    $pub_opts = "$opts -t default_tcp";
    $sub_opts = "$opts -t default_tcp";
}
elsif ($ARGV[0] eq 'default_udp') {
    $opts = $use_svc_conf ? "-ORBSvcConf udp.conf" : '';
    $pub_opts = "$opts -t default_udp";
    $sub_opts = "$opts -t default_udp";
}
elsif ($ARGV[0] eq 'default_mcast') {
    $opts = $use_svc_conf ? "-ORBSvcConf mcast.conf" : '';
    $pub_opts = "$opts -t default_mcast_pub";
    $sub_opts = "$opts -t default_mcast_sub";
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

$repo_svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
                 : "-ORBSvcConf ../../tcp.conf";

$domains_file = "domain_ids";
$dcpsrepo_ior = "repo.ior";
$info_prst_file = "info.pr";
$repo_bit_opt = "$repo_svc_config -NOBITS";
$app_bit_opt = "-DCPSBit 0";
$SRV_PORT = PerlACE::random_port();

unlink $dcpsrepo_ior;
unlink $info_prst_file;

# If InfoRepo is running in persistent mode, use a
#  static endpoint (instead of transient)
$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file "
                                    . "-ORBSvcConf mySvc.conf "
                                    . "-orbendpoint iiop://:$SRV_PORT");
$Subscriber = PerlDDS::create_process ("subscriber", "$app_bit_opt $sub_opts");
$Publisher = PerlDDS::create_process ("publisher", "$app_bit_opt $pub_opts");

print "Spawning first DCPSInfoRepo.\n";
print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print "Spawning publisher.\n";
print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print "Spawning first subscriber.\n";
print $Subscriber->CommandLine() . "\n";
$SubscriberResult = $Subscriber->SpawnWaitKill (30);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}
print "First Subscriber complete.\n";

print "Killing first DCPSInfoRepo.\n";
$ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}
unlink $dcpsrepo_ior;

print "Spawning second DCPSInfoRepo.\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print "Spawning second subscriber.\n";
$SubscriberResult = $Subscriber->SpawnWaitKill (60);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}
print "Second Subscriber complete.\n";

$PublisherResult = $Publisher->WaitKill (15);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}
print "Publisher killed.\n";

print "Killing second DCPSInfoRepo.\n";
$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}
unlink $dcpsrepo_ior;
unlink $info_prst_file;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
