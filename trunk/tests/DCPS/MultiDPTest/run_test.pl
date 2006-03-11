eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../FooType5');

# single reader with single instances test

$num_writers=1;
$num_instances_per_writer=1;
$num_samples_per_instance=10;
$sub_addr1 = "localhost:16701";
$sub_addr2 = "localhost:16702";
$pub_addr = "localhost:29803";
$sequence_length=10;

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");

$subscriber_completed = PerlACE::LocalFile ("subscriber_finished.txt");
$subscriber_ready = PerlACE::LocalFile ("subscriber_ready.txt");
$publisher_completed = PerlACE::LocalFile ("publisher_finished.txt");
$publisher_ready = PerlACE::LocalFile ("publisher_ready.txt");

unlink $dcpsrepo_ior; 
unlink $subscriber_completed; 
unlink $subscriber_ready; 
unlink $publisher_completed; 
unlink $publisher_ready; 


$DCPSREPO = new PerlACE::Process ("../../../dds/InfoRepo/DCPSInfoRepo",
                             "-NOBITS -o $dcpsrepo_ior"
#                             . " -ORBDebugLevel 1"
                             . " -d $domains_file");
print $DCPSREPO->CommandLine(), "\n";

# test multiple cases
$sub_parameters = "-s $sub_addr1 -s $sub_addr2 "
              . " -m $num_instances_per_writer -i $num_samples_per_instance";
              
$Subscriber = new PerlACE::Process ("subscriber", $sub_parameters);
print $Subscriber->CommandLine(), "\n";

$pub_parameters = "-p $pub_addr "
              . " -m $num_instances_per_writer -i $num_samples_per_instance";


$Publisher = new PerlACE::Process ("publisher", $pub_parameters);
print $Publisher->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$Publisher->Spawn ();

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
unlink $subscriber_completed; 
unlink $subscriber_ready; 
unlink $publisher_completed; 
unlink $publisher_ready; 

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;






