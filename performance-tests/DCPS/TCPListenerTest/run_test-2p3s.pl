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
$num_writers=2;
$num_readers=3;
$num_msgs_btwn_rec=20;
$pub_writer_id=0;
$repo_bit_conf = "-NOBITS ";
$app_bit_conf = "-DCPSBit 0 ";
$use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');

if ($ARGV[0] eq 'bit') {
  $repo_bit_conf = $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '' ;
  $app_bit_conf = $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '' ;
}
elsif ($ARGV[0] eq '') {
  # default test with bit off
  $repo_bit_conf .= $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '' ;
  $app_bit_conf .= $use_svc_config ? "-ORBSvcConf ../../tcp.conf" : '' ;
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}



# need $num_msgs_btwn_rec unread samples plus 20 for good measure
# (possibly allocated by not yet queue by the transport because of greedy read).
$num_samples=$num_msgs_btwn_rec + 20;

$domains_file = "domain_ids";
$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf -o $dcpsrepo_ior"
                             . " -d $domains_file");

print $DCPSREPO->CommandLine(), "\n";

$svc_config = ($use_svc_config && $app_bit_conf eq '')
    ? " -ORBSvcConf ../../tcp.conf " : '';
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

$Sub1 = PerlDDS::create_process ("subscriber", $sub_parameters);
print $Sub1->CommandLine(), "\n";
$Sub2 = PerlDDS::create_process ("subscriber", $sub_parameters);
print $Sub2->CommandLine(), "\n";
$Sub3 = PerlDDS::create_process ("subscriber", $sub_parameters);
print $Sub3->CommandLine(), "\n";

#NOTE: above 1000 queue samples does not give any better performance.
$pub_parameters = "$app_bit_conf -DCPSConfigFile conf.ini "
#              . " -DCPSDebugLevel 6"
   . "$svc_config"
              . " -p 1"
              . " -r $num_readers"
              . " -n $num_messages -d $data_size"
              . " -msi 1000 -mxs 1000";

$Pub1 = PerlDDS::create_process ("publisher", $pub_parameters . " -i $pub_writer_id");
print $Pub1->CommandLine(), "\n";
$pub_writer_id++;
$Pub2 = PerlDDS::create_process ("publisher", $pub_parameters . " -i $pub_writer_id");
print $Pub2->CommandLine(), "\n";
$pub_writer_id++;


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$Sub1->Spawn ();
$Sub2->Spawn ();
$Sub3->Spawn ();

$Pub1->Spawn ();
$Pub2->Spawn ();


$wait_to_kill = 200;
$Pub1Result = $Pub1->WaitKill ($wait_to_kill);
if ($Pub1Result != 0) {
    print STDERR "ERROR: publisher 1 returned $Pub1Result \n";
    $status = 1;
    $wait_to_kill = 0;
}


$Pub2Result = $Pub2->WaitKill ($wait_to_kill);
if ($Pub2Result != 0) {
    print STDERR "ERROR: publisher 2 returned $Pub2Result \n";
    $status = 1;
    $wait_to_kill = 0;
}



$Sub1Result = $Sub1->WaitKill ($wait_to_kill);
if ($Sub1Result != 0) {
    print STDERR "ERROR: subscriber 1 returned $Sub1Result\n";
    $status = 1;
    $wait_to_kill = 0;
}


$Sub2Result = $Sub2->WaitKill ($wait_to_kill);
if ($Sub2Result != 0) {
    print STDERR "ERROR: subscriber 2 returned $Sub2Result \n";
    $status = 1;
    $wait_to_kill = 0;
}


$Sub3Result = $Sub3->WaitKill ($wait_to_kill);
if ($Sub3Result != 0) {
    print STDERR "ERROR: subscriber 3 returned $Sub3Result \n";
    $status = 1;
    $wait_to_kill = 0;
}


$ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
