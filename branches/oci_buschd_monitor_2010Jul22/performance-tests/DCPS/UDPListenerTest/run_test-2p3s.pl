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

PerlDDS::add_lib_path('../TypeNoKeyBounded');


# single reader with single instances test
$num_messages=500;
$data_size=13;
$num_writers=2;
$num_readers=3;
$pub_addr_host="localhost";
$pub_addr_port=34567;
$sub_addr_host="localhost";
$sub_addr_port=45678;
$num_msgs_btwn_rec=1;
$pub_writer_id=0;
$write_throttle=300000*$num_writers;

$dcpsrepo_ior = "repo.ior";
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-DCPSBit 0";

unlink $dcpsrepo_ior; 

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf -o $dcpsrepo_ior ");


print $DCPSREPO->CommandLine(), "\n";


$sub_parameters = "-ORBSvcConf udp.conf $app_bit_conf -p $num_writers"
#              . " -DCPSDebugLevel 6"
              . " -i $num_msgs_btwn_rec"
              . " -n $num_messages -d $data_size"
              . " -msi $num_messages -mxs $num_messages";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap 
#   (could be less than $num_messages but I am not sure of the limit).

$Sub1 = PerlDDS::create_process ("subscriber", $sub_parameters . " -a $sub_addr_host:$sub_addr_port");
print $Sub1->CommandLine(), "\n";
$sub_addr_port++;

$Sub2 = PerlDDS::create_process ("subscriber", $sub_parameters . " -a $sub_addr_host:$sub_addr_port");
print $Sub2->CommandLine(), "\n";
$sub_addr_port++;

$Sub3 = PerlDDS::create_process ("subscriber", $sub_parameters . " -a $sub_addr_host:$sub_addr_port");
print $Sub3->CommandLine(), "\n";
$sub_addr_port++;


$pub_parameters = "-ORBSvcConf udp.conf $app_bit_conf -p 1"
#              . " -DCPSDebugLevel 6"
              . " -r $num_readers" 
              . " -n $num_messages -d $data_size" 
              . " -msi 1000 -mxs 1000 -h $write_throttle";

$Pub1 = PerlDDS::create_process ("publisher", $pub_parameters . "  -i $pub_writer_id -a $pub_addr_host:$pub_addr_port");
print $Pub1->CommandLine(), "\n";
$pub_addr_port++;
$pub_writer_id++;

$Pub2 = PerlDDS::create_process ("publisher", $pub_parameters . "  -i $pub_writer_id -a $pub_addr_host:$pub_addr_port");
print $Pub2->CommandLine(), "\n";
$pub_addr_port++;
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



$Sub1Result = $Sub1->WaitKill (1200);
if ($Sub1Result != 0) {
    print STDERR "ERROR: subscriber 1 returned $Sub1Result\n";
    $status = 1;
}


$Sub2Result = $Sub2->WaitKill (1200);
if ($Sub2Result != 0) {
    print STDERR "ERROR: subscriber 2 returned $Sub2Result \n";
    $status = 1;
}


$Sub3Result = $Sub3->WaitKill (1200);
if ($Sub3Result != 0) {
    print STDERR "ERROR: subscriber 3 returned $Sub3Result \n";
    $status = 1;
}



$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
