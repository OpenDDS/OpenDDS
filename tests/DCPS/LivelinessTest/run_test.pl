eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../FooType4');
PerlACE::add_lib_path('../common');

$subscriber_completed = PerlACE::LocalFile ("subscriber_finished.txt");
$subscriber_ready = PerlACE::LocalFile ("subscriber_ready.txt");
$publisher_completed = PerlACE::LocalFile ("publisher_finished.txt");
$publisher_ready = PerlACE::LocalFile ("publisher_ready.txt");

unlink $subscriber_completed; 
unlink $subscriber_ready; 
unlink $publisher_completed; 
unlink $publisher_ready; 

# single reader with single instances test
$multiple_instance=0;
$num_samples_per_reader=2;
$num_unlively_periods=3;
$num_readers=1;
$use_take = 0;
$use_udp = 0;
$sub_addr = "localhost:16701";
$pub_addr = "localhost:29803";

$arg_idx = 0;

if ($ARGV[0] eq 'udp') {
  $use_udp = 1;
  $arg_idx = 1;
  $svc_conf = " -ORBSvcConf udp.conf ";
}

if ($ARGV[$arg_idx] eq 'take') { 
  print "use_take !!!!!\n";
  $use_take = 1;
}
elsif ($ARGV[$arg_idx] eq '') { 
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[$arg_idx] $arg_idx\n";
  exit 1;
}

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
$repo_bit_conf = "-ORBSvcConf ../../tcp.conf";

unlink $dcpsrepo_ior; 

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                             "$repo_bit_conf "
#                           . "-ORBDebugLevel 1 "
                           . "-o $dcpsrepo_ior");


print $DCPSREPO->CommandLine(), "\n";
$common_parameters = "-u $use_udp"
              . " -w $num_readers -m $multiple_instance"
              . " -l $num_unlively_periods -i $num_samples_per_reader " ;
              
# test multiple cases
$sub_parameters = "$svc_conf $common_parameters -s $sub_addr -t $use_take ";

$Subscriber = new PerlACE::Process ("subscriber", $sub_parameters);
print $Subscriber->CommandLine(), "\n";

$pub_parameters = "$svc_conf $common_parameters -p $pub_addr" ;

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

$PublisherResult = $Publisher->WaitKill (60); #TBD REMOVE change back to 300

if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}
$SubscriberResult = $Subscriber->WaitKill (30);

if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


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
