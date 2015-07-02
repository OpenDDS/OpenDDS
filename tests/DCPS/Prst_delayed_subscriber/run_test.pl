eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

########################################
#
my $debug ;# = 10;
my $repoDebug;
my $pubDebug;
my $subDebug;
$repoDebug = $debug if not $repoDebug and $debug;
$pubDebug  = $debug if not $pubDebug  and $debug;
$subDebug  = $debug if not $subDebug  and $debug;

my $transportDebug ;# = 10;
my $repoTransportDebug;
my $pubTransportDebug;
my $subTransportDebug;
$repoTransportDebug = $debug if not $repoTransportDebug and $transportDebug;
$pubTransportDebug  = $debug if not $pubTransportDebug  and $transportDebug;
$subTransportDebug  = $debug if not $subTransportDebug  and $transportDebug;

my $debugFile;

my $repoDebugOpts = "";
$repoDebugOpts .= "-DCPSTransportDebugLevel $repoTransportDebug " if $repoTransportDebug;
$repoDebugOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoDebugOpts .= "-ORBLogFile $debugFile "     if $repoDebug and $debugFile;

my $pubDebugOpts = "";
$pubDebugOpts .= "-DCPSTransportDebugLevel $pubTransportDebug " if $pubTransportDebug;
$pubDebugOpts .= "-DCPSDebugLevel $pubDebug " if $pubDebug;
$pubDebugOpts .= "-ORBLogFile $debugFile "    if $pubDebug and $debugFile;

my $subDebugOpts = "";
$subDebugOpts .= "-DCPSTransportDebugLevel $subTransportDebug " if $subTransportDebug;
$subDebugOpts .= "-DCPSDebugLevel $subDebug " if $subDebug;
$subDebugOpts .= "-ORBLogFile $debugFile "    if $subDebug and $debugFile;
#
########################################


$pub_opts = "$pubDebugOpts -DCPSConfigFile pub.ini";
$sub_opts = "$subDebugOpts -DCPSConfigFile sub.ini";

if ($ARGV[0] eq 'udp') {
    $opts = "-t udp";
    $pub_opts = "$opts -DCPSConfigFile pub_udp.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'multicast') {
    $opts = "-t multicast";
    $pub_opts = "$opts -DCPSConfigFile pub_multicast.ini";
    $sub_opts = "$opts -DCPSConfigFile sub_multicast.ini";
}
elsif ($ARGV[0] eq 'default_tcp') {
    $pub_opts = "-t default_tcp";
    $sub_opts = "-t default_tcp";
}
elsif ($ARGV[0] eq 'default_udp') {
    $pub_opts = "-t default_udp";
    $sub_opts = "-t default_udp";
}
elsif ($ARGV[0] eq 'default_multicast') {
    $pub_opts = "-t default_multicast";
    $sub_opts = "-t default_multicast";
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}


$dcpsrepo_ior = "repo.ior";
$info_prst_file = "info.pr";
$repo_bit_opt = "-NOBITS";
$app_bit_opt = "-DCPSBit 0";
$SRV_PORT = PerlACE::random_port();

unlink $dcpsrepo_ior;
unlink $info_prst_file;

# If InfoRepo is running in persistent mode, use a
#  static endpoint (instead of transient)
$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$repo_bit_opt $repoDebugOpts -o $dcpsrepo_ior "
                                    . "-ORBSvcConf mySvc.conf "
                                    . "-orbendpoint iiop://:$SRV_PORT");
$Subscriber = PerlDDS::create_process ("subscriber", "$app_bit_opt $sub_opts");
$Publisher = PerlDDS::create_process ("publisher", "$app_bit_opt $pub_opts");

print "Spawning first DCPSInfoRepo.\n";
print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print "Spawning publisher.\n";
print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

# Attempt to ensure that the Publisher has the first participant value to
# test that the participant numbering is reset during the persistence
# update/download/whatEVER.
sleep 5;

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
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
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
