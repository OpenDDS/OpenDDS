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
$num_messages=100;
$data_size=13;
$num_msgs_btwn_rec=1;
$pub_writer_id=0;
$write_throttle=300000*$num_writers;
$local_address='224.0.0.1:29803';

$dcpsrepo_ior = "repo.ior";
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-DCPSBit 0";
$num_writers_per_pub=4;
$num_readers_per_sub=1;
$num_subs = 4;
$num_pubs = 16;
$num_writers = $num_pubs * $num_writers_per_pub;
$num_readers = $num_subs * $num_readers_per_sub;

unlink $dcpsrepo_ior; 

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf -o $dcpsrepo_ior ");


print $DCPSREPO->CommandLine(), "\n";


$sub_parameters = "-ORBSvcConf multicast.conf $app_bit_conf -p $num_writers"
#              . " -DCPSDebugLevel 6"
              . " -ORBVerboseLogging 1 -i $num_msgs_btwn_rec"
              . " -n $num_messages -d $data_size"
              . " -msi $num_messages -mxs $num_messages -multicast $local_address";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap 
#   (could be less than $num_messages but I am not sure of the limit).

for ($n = 0; $n < $num_subs; ++$n) {
  $Sub[$n] = PerlDDS::create_process ("subscriber", $sub_parameters );
  print $Sub[$n]->CommandLine(), "\n";
}

$pub_parameters = "-ORBSvcConf multicast.conf $app_bit_conf -p $num_writers_per_pub"
#              . " -DCPSDebugLevel 6"
              . " -ORBVerboseLogging 1 -r $num_readers" 
              . " -n $num_messages -d $data_size" 
              . " -msi 1000 -mxs 1000 -h $write_throttle -multicast $local_address";

for ($n = 0; $n < $num_pubs; ++$n) {
  $Pub[$n] = PerlDDS::create_process ("publisher", $pub_parameters . " -i $pub_writer_id ");
  $pub_writer_id += $num_writers_per_pub;
  print $Pub[$n]->CommandLine(), "\n";
}

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


for ($n = 0; $n < $num_subs; ++$n) {
  $SubResult = $Sub[$n]->Spawn ();
  if ($SubResult != 0) {
      print STDERR "ERROR: Subscriber $n returned $SubResult \n";
      $status = 1;
  }
}

for ($n = 0; $n < $num_pubs; ++$n) {
  print STDERR " start pub $n\n";
  $PubResult = $Pub[$n]->Spawn ();
  if ($PubResult != 0) {
      print STDERR "ERROR: publisher $n returned $PubResult \n";
      $status = 1;
  }
}



for ($n = 0; $n < $num_subs; ++$n) {
  $SubResult = $Sub[$n]->WaitKill (1200);
  if ($SubResult != 0) {
      print STDERR "ERROR: publisher $n returned $SubResult \n";
      $status = 1;
  }
}


for ($n = 0; $n < $num_pubs; ++$n) {
  $PubResult = $Pub[$n]->WaitKill (1200);
  if ($PubResult != 0) {
      print STDERR "ERROR: publisher $n returned $PubResult \n";
      $status = 1;
  }
}



$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
