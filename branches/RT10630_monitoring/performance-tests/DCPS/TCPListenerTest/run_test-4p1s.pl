eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../TypeNoKeyBounded');


# single reader with single instances test
$num_messages=500;
$data_size=13;
$num_writers=4;
$num_readers=1;
$num_msgs_btwn_rec=20;
$pub_writer_id=0;
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-DCPSBit 0";

if ($ARGV[0] eq 'bit') {
  $repo_bit_conf = "-ORBSvcConf ../../tcp.conf";
  $app_bit_conf = "";
}
elsif ($ARGV[0] eq '') {
  # default test with bit off
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}


# need $num_msgs_btwn_rec unread samples plus 20 for good measure
# (possibly allocated by not yet queue by the transport because of greedy read).
$num_samples=$num_msgs_btwn_rec + 20;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");

unlink $dcpsrepo_ior;

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf -o $dcpsrepo_ior"
                             . " -d $domains_file ");

print $DCPSREPO->CommandLine(), "\n";

$svc_config=" -ORBSvcConf ../../tcp.conf ";
$sub_parameters = "$app_bit_conf -DCPSConfigFile conf.ini "
#              . " -DCPSDebugLevel 6"
   . "$svc_config"
              . "  -p $num_writers"
              . " -i $num_msgs_btwn_rec"
              . " -n $num_messages -d $data_size"
              . " -msi $num_samples -mxs $num_samples";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap
#   (could be less than $num_messages but I am not sure of the limit).

$Sub1 = new PerlACE::Process ("subscriber", $sub_parameters);
print $Sub1->CommandLine(), "\n";


#NOTE: above 1000 queue samples does not give any better performance.
$pub_parameters = "$app_bit_conf -DCPSConfigFile conf.ini "
#              . " -DCPSDebugLevel 6"
   . "$svc_config"
              . " -p 1"
              . " -r $num_readers"
              . " -n $num_messages -d $data_size"
              . " -msi 1000 -mxs 1000";

$Pub1 = new PerlACE::Process ("publisher", $pub_parameters . " -i $pub_writer_id");
print $Pub1->CommandLine(), "\n";
$pub_writer_id++;
$Pub2 = new PerlACE::Process ("publisher", $pub_parameters . " -i $pub_writer_id");
print $Pub2->CommandLine(), "\n";
$pub_writer_id++;
$Pub3 = new PerlACE::Process ("publisher", $pub_parameters . " -i $pub_writer_id");
print $Pub3->CommandLine(), "\n";
$pub_writer_id++;
$Pub4 = new PerlACE::Process ("publisher", $pub_parameters . " -i $pub_writer_id");
print $Pub4->CommandLine(), "\n";
$pub_writer_id++;


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$Sub1->Spawn ();


$Pub1->Spawn ();
$Pub2->Spawn ();
$Pub3->Spawn ();
$Pub4->Spawn ();


$Pub1Result = $Pub1->WaitKill (1200);
if ($Pub1Result != 0) {
    print STDERR "ERROR: publisher 1 returned $Pub1Result \n";
    $status = 1;
}


$Pub2Result = $Pub2->WaitKill (1200);
if ($Pub2Result != 0) {
    print STDERR "ERROR: publisher 2 returned $Pub2Result \n";
    $status = 1;
}


$Pub3Result = $Pub3->WaitKill (1200);
if ($Pub3Result != 0) {
    print STDERR "ERROR: publisher 3 returned $Pub3Result \n";
    $status = 1;
}


$Pub4Result = $Pub4->WaitKill (1200);
if ($Pub4Result != 0) {
    print STDERR "ERROR: publisher 4 returned $Pub4Result \n";
    $status = 1;
}



$Sub1Result = $Sub1->WaitKill (1200);
if ($Sub1Result != 0) {
    print STDERR "ERROR: subscriber 1 returned $Sub1Result\n";
    $status = 1;
}



$ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
