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

PerlDDS::add_lib_path('../TypeNoKeyBounded');


# single reader with single instances test
$num_messages=5000;
$data_size=7;
$num_writers=2;
$num_readers=1;
$pub1_addr='localhost:34567';
$pub2_addr='localhost:34568';
$sub_addr='localhost:45678';

$dcpsrepo_ior = "repo.ior";
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-DCPSBit 0";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf -o $dcpsrepo_ior ");


print $DCPSREPO->CommandLine(), "\n";
$sub_parameters = "-DCPSConfigFile conf.ini $app_bit_conf -p $num_writers"
#              . " -DCPSDebugLevel 6"
              . " -n $num_messages -d $data_size"
              . " -msi $num_messages -mxs $num_messages";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap
#   (could be less than $num_messages but I am not sure of the limit).

$Subscriber = PerlDDS::create_process ("subscriber", $sub_parameters);
print $Subscriber->CommandLine(), "\n";

$pub1_parameters = "-DCPSConfigFile conf.ini $app_bit_conf -p 1"
#              . " -DCPSDebugLevel 6"
              . " -n $num_messages -d $data_size"
              . " -msi 1000 -mxs 1000 -i 0 -h 225000";

$Publisher1 = PerlDDS::create_process ("publisher", $pub1_parameters);
print $Publisher1->CommandLine(), "\n";

$pub2_parameters = "-DCPSConfigFile conf.ini $app_bit_conf -p 1"
#              . " -DCPSDebugLevel 6"
              . " -n $num_messages -d $data_size"
              . " -msi 1000 -mxs 1000 -i 1 -h 225000";

$Publisher2 = PerlDDS::create_process ("publisher", $pub2_parameters);
print $Publisher2->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$Publisher1->Spawn ();

$Publisher2->Spawn ();

$Subscriber->Spawn ();

$Publisher1Result = $Publisher1->WaitKill (1200);
if ($Publisher1Result != 0) {
    print STDERR "ERROR: publisher returned $Publisher1Result \n";
    $status = 1;
}

$Publisher2Result = $Publisher2->WaitKill (1200);
if ($Publisher2Result != 0) {
    print STDERR "ERROR: publisher returned $Publisher2Result \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (1200);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;
