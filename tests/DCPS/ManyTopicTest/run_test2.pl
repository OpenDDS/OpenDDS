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

PerlDDS::add_lib_path('../ManyTopicTypes');
PerlDDS::add_lib_path('../common');

$subscriber1_completed = "T1_subscriber_finished.txt";
$subscriber2_completed = "T2_subscriber_finished.txt";
$subscriber3_completed = "T3_subscriber_finished.txt";
#$subscriber_ready = "subscriber_ready.txt";

$publisher1_completed = "T1_publisher_finished.txt";
$publisher2_completed = "T2_publisher_finished.txt";
$publisher3_completed = "T3_publisher_finished.txt";
#$publisher_ready = "publisher_ready.txt";

unlink $subscriber1_completed;
unlink $subscriber2_completed;
unlink $subscriber3_completed;
#unlink $subscriber_ready;
unlink $publisher1_completed;
unlink $publisher2_completed;
unlink $publisher3_completed;
#unlink $publisher_ready;

# single reader with single instances test
$multiple_instance=0;
$num_samples_per_reader=10;
$num_readers=1;
$use_take=0;
$use_udp = 0;

$arg_idx = 0;

if ($ARGV[0] eq 'udp') {
  $use_udp = 1;
  $arg_idx = 1;
}


# multiple instances test
if ($ARGV[$arg_idx] eq 'mi') {
  $multiple_instance=1;
  $num_samples_per_reader=10;
  $num_readers=1;
}
# multiple datareaders with single instance test
elsif ($ARGV[$arg_idx] eq 'mr') {
  $multiple_instance=0;
  $num_samples_per_reader=5;
  $num_readers=2;
}
# multiple datareaders with multiple instances test
elsif ($ARGV[$arg_idx] eq 'mri') {
  $multiple_instance=1;
  $num_samples_per_reader=4;
  $num_readers=3;
}
# multiple datareaders with multiple instances test
elsif ($ARGV[$arg_idx] eq 'mrit') {
  $multiple_instance=1;
  $num_samples_per_reader=4;
  $num_readers=3;
  $use_take=1;
}
elsif ($ARGV[$arg_idx] eq '') {
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[$arg_idx] $arg_idx\n";
  exit 1;
}

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

# test multiple cases
$sub1_parameters = "-t 1" ;
$sub2_parameters = "-t 2" ;
$sub3_parameters = "-t 3" ;
$pub_parameters = " -t all " ;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                     "-o $dcpsrepo_ior");


print $DCPSREPO->CommandLine(), "\n";

$Subscriber1 = PerlDDS::create_process ("subscriber", $sub1_parameters);
print $Subscriber1->CommandLine(), "\n";

$Subscriber2 = PerlDDS::create_process ("subscriber", $sub2_parameters);
print $Subscriber2->CommandLine(), "\n";

$Subscriber3 = PerlDDS::create_process ("subscriber", $sub3_parameters);
print $Subscriber3->CommandLine(), "\n";

$Publisher = PerlDDS::create_process ("publisher", $pub_parameters);
print $Publisher->CommandLine(), "\n";

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$Publisher->Spawn ();

$Subscriber1->Spawn ();
$Subscriber2->Spawn ();
$Subscriber3->Spawn ();

$PublisherResult = $Publisher->WaitKill (300);

if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$Subscriber1Result = $Subscriber1->WaitKill (30);
if ($Subscriber1Result != 0) {
    print STDERR "ERROR: subscriber 1 returned $Subscriber1Result \n";
    $status = 1;
}

$Subscriber2Result = $Subscriber2->WaitKill (10);
if ($Subscriber2Result != 0) {
    print STDERR "ERROR: subscriber 2 returned $Subscriber2Result \n";
    $status = 1;
}

$Subscriber3Result = $Subscriber3->WaitKill (10);
if ($Subscriber3Result != 0) {
    print STDERR "ERROR: subscriber 3 returned $Subscriber3Result \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


unlink $subscriber1_completed;
unlink $subscriber2_completed;
unlink $subscriber3_completed;
#unlink $subscriber_ready;
unlink $publisher1_completed;
unlink $publisher2_completed;
unlink $publisher3_completed;
#unlink $publisher_ready;

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
