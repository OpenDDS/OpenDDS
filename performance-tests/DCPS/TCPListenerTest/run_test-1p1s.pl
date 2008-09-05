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

PerlDDS::add_lib_path('../TypeNoKeyBounded');


# single reader with single instances test
$num_messages=500;
$data_size=13;
$num_writers=1;
$num_readers=1;
$num_msgs_btwn_rec=20;
$pub_writer_id=0;
$repo_bit_conf = "-NOBITS ";
$app_bit_conf = "-DCPSBit 0 ";
$copy_sample=0;
$use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');

if ($ARGV[0] ne '') {
    $data_size = $ARGV[0];
}

if ($ARGV[1] ne '') {
    $copy_sample = $ARGV[1];
}

if ($ARGV[2] ne '') {
    $num_messages = $ARGV[2];
}

if ($ARGV[3] eq 'bit') {
  $repo_bit_conf = $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '';
  $app_bit_conf = $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '';
}
elsif ($ARGV[3] eq '' or $ARGV[3] eq 'nobit') {
  # default test with bit off
  $repo_bit_conf .= $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '';
  $app_bit_conf .= $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '';
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[3] \n";
  exit 1;
}

# need $num_msgs_btwn_rec unread samples plus 20 for good measure
# (possibly allocated by not yet queue by the transport because of greedy read).
$num_samples=$num_msgs_btwn_rec + 20;

$dcpsrepo_ior = "repo.ior";

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

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf $repoDebugOpts -o $dcpsrepo_ior ");


print $DCPSREPO->CommandLine(), "\n";

$svc_config = ($use_svc_config && $app_bit_conf eq '')
    ? " -ORBSvcConf ../../tcp.conf " : '';
$sub_parameters = "$app_bit_conf $subDebugOpts -DCPSConfigFile conf.ini -p $num_writers"
              . "$svc_config"
              . " -i $num_msgs_btwn_rec"
              . " -n $num_messages -d $data_size"
              . " -msi $num_samples -mxs $num_samples"
              . " -c $copy_sample";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap
#   (could be less than $num_messages but I am not sure of the limit).

$Subscriber = PerlDDS::create_process ("subscriber", $sub_parameters);
print $Subscriber->CommandLine(), "\n";


#NOTE: above 1000 queue samples does not give any better performance.
$pub_parameters = "$app_bit_conf $pubDebugOpts -DCPSConfigFile conf.ini -p 1 -i $pub_writer_id"
#              . " -DCPSDebugLevel 6"
   . "$svc_config"
              . " -n $num_messages -d $data_size"
              . " -msi 1000 -mxs 1000";

$Publisher = PerlDDS::create_process ("publisher", $pub_parameters);
print $Publisher->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$Publisher->Spawn ();

$Subscriber->Spawn ();

$wait_to_kill = 200;
$PublisherResult = $Publisher->WaitKill ($wait_to_kill);

if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
    $wait_to_kill = 0;
}
$SubscriberResult = $Subscriber->WaitKill ($wait_to_kill);

if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(10);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
