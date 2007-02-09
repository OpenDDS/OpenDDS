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
$num_messages=1000;
$data_size=13;
$num_writers=1;
$num_readers=4;
$num_msgs_btwn_rec=1;
$pub_writer_id=0;
$write_throttle=300000*$num_writers;
$mcast_addr='224.0.0.1:29803';

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");

unlink $dcpsrepo_ior; 

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "-NOBITS -o $dcpsrepo_ior"
                             . " -d $domains_file");


print $DCPSREPO->CommandLine(), "\n";


$sub_parameters = "-ORBSvcConf  mcast.conf -p $num_writers"
#              . " -DCPSDebugLevel 6"
              . " -i $num_msgs_btwn_rec"
              . " -n $num_messages -d $data_size"
              . " -msi $num_messages -mxs $num_messages -mcast $mcast_addr";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap 
#   (could be less than $num_messages but I am not sure of the limit).

$Sub1 = new PerlACE::Process ("subscriber", $sub_parameters); 
print $Sub1->CommandLine(), "\n";
$sub_addr_port++;

$Sub2 = new PerlACE::Process ("subscriber", $sub_parameters );
print $Sub2->CommandLine(), "\n";

$Sub3 = new PerlACE::Process ("subscriber", $sub_parameters );
print $Sub3->CommandLine(), "\n";

$Sub4 = new PerlACE::Process ("subscriber", $sub_parameters );
print $Sub4->CommandLine(), "\n";

$pub_parameters = "-ORBSvcConf  mcast.conf -p 1"
#              . " -DCPSDebugLevel 6"
              . " -r $num_readers" 
              . " -n $num_messages -d $data_size" 
              . " -msi 1000 -mxs 1000 -h $write_throttle -mcast $mcast_addr";

$Pub1 = new PerlACE::Process ("publisher", $pub_parameters . " -i $pub_writer_id "); #-a $pub_addr_host:$pub_addr_port");
print $Pub1->CommandLine(), "\n";
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
$Sub4->Spawn ();

$Pub1->Spawn ();


$Pub1Result = $Pub1->WaitKill (1200);
if ($Pub1Result != 0) {
    print STDERR "ERROR: publisher 1 returned $Pub1Result \n";
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


$Sub4Result = $Sub4->WaitKill (1200);
if ($Sub4Result != 0) {
    print STDERR "ERROR: subscriber 4 returned $Sub4Result \n";
    $status = 1;
}



$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
