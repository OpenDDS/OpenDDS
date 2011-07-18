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

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

$subscriber_completed = "subscriber_finished.txt";
$subscriber_ready = "subscriber_ready.txt";
$publisher_completed = "publisher_finished.txt";
$publisher_ready = "publisher_ready.txt";

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

$arg_idx = 0;

$config_file = "tcp.ini";

if ($ARGV[0] eq 'udp') {
    $config_file = "udp.ini";
  $arg_idx = 1;
  $app_bit_conf = " -DCPSBit 0 "
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

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                     "-o $dcpsrepo_ior "
#                                    . "-ORBDebugLevel 1 "
                                     );


print $DCPSREPO->CommandLine(), "\n";
$common_parameters = "-DCPSConfigFile $config_file $app_bit_conf"
              . " -w $num_readers -m $multiple_instance"
              . " -l $num_unlively_periods -i $num_samples_per_reader " ;

# test multiple cases
$sub_parameters = "$common_parameters -t $use_take ";

$pub_parameters = "$common_parameters " ;

$Subscriber = PerlDDS::create_process ("subscriber", $sub_parameters);
print $Subscriber->CommandLine(), "\n";

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
