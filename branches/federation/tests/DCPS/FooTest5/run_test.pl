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

PerlDDS::add_lib_path('../FooType5');

# single reader with single instances test

$num_writers=1;
$num_instances_per_writer=1;
$num_samples_per_instance=100;
$num_writers=1;
$use_take=0;
$use_udp = 0;
$use_mcast = 0;
$use_reliable_multicast = 0;
$sub_addr = "localhost:16701";
$pub_addr = "localhost:29803";
$num_readers=1;
$max_samples_per_instance=1000;
$sequence_length=10;
$no_key = 0;
$write_interval_ms=0;
$writer_blocking_ms=0;
$read_interval_ms=0;
$mixed_trans=0;
$repo_bit_conf = "-ORBSvcConf ../../tcp.conf";
$app_bit_conf = "-ORBSvcConf ../../tcp.conf";

$arg_idx = 0;

if ($ARGV[$arg_idx] eq 'nokey') {
  $no_key = 1;
  $arg_idx = $arg_idx + 1;
}

if ($ARGV[$arg_idx] eq 'udp') {
  $use_udp = 1;
  $arg_idx = $arg_idx + 1;
  $write_interval_ms = 50;
}

if ($ARGV[$arg_idx] eq 'mcast') {
  $use_mcast = 1;
  $pub_addr = "224.0.0.1:29803";
  $arg_idx = $arg_idx + 1;
  $write_interval_ms = 50;
}

if ($ARGV[$arg_idx] eq 'reliable_multicast') {
  $use_reliable_multicast = 1;
  $pub_addr = "224.0.0.1:29804";
  $arg_idx = $arg_idx + 1;
}


# multiple instances, single datawriter, single datareader
if ($ARGV[$arg_idx] eq 'mi') {
  $num_instances_per_writer=2;
}
elsif ($ARGV[$arg_idx] eq 'mwmr') {
  $num_samples_per_instance=50;
  $num_writers = 2;
  $num_readers = 2;
}
elsif ($ARGV[$arg_idx] eq 'mwmr_long_seq') {
  $sequence_length=256;
  $num_samples_per_instance=50;
  $num_writers = 2;
  $num_readers = 2;
}
elsif ($ARGV[$arg_idx] eq 'blocking') {
  $writer_blocking_ms=1000000; # 1000 seconds
  $num_instances_per_writer=5;
  $num_samples_per_instance=20;
  $max_samples_per_instance=1;
  $read_interval_ms=1000;
}
elsif ($ARGV[$arg_idx] eq 'blocking_timeout') {
  $writer_blocking_ms=5; # milliseconds
  $num_instances_per_writer=5;
  $num_samples_per_instance=20;
  $max_samples_per_instance=1;
  $read_interval_ms=1000;
}
elsif ($ARGV[$arg_idx] eq 'mixed_trans') {
  $num_samples_per_instance=50;
  $num_writers = 2;
  $num_readers = 2;
  $mixed_trans = 1;
}
elsif ($ARGV[$arg_idx] eq '') {
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[$arg_idx] \n";
  exit 1;
}

$dcpsrepo_ior = "repo.ior";

$subscriber_completed = "subscriber_finished.txt";
$subscriber_ready = "subscriber_ready.txt";
$publisher_completed = "publisher_finished.txt";
$publisher_ready = "publisher_ready.txt";

unlink $dcpsrepo_ior;
unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_completed;
unlink $publisher_ready;

$use_svc_config = !new PerlACE::ConfigList->check_config ('STATIC');
$tcp_svc_config = $use_svc_config ? " -ORBSvcConf ../../tcp.conf " : '';

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$tcp_svc_config -o $dcpsrepo_ior ");
print $DCPSREPO->CommandLine(), "\n";

$bit_off_conf = "-DCPSBit 0";
$svc_config = $tcp_svc_config;
if ($use_udp == 1) {
  $svc_config = ($use_svc_config ? " -ORBSvcConf udp.conf " : '')
      . $bit_off_conf;
}
elsif ($mixed_trans == 1) {
  $svc_config .= ($use_svc_config ? " -ORBSvcConf udp.conf " : '')
      . $bit_off_conf;
}
elsif ($use_mcast == 1) {
  $svc_config = ($use_svc_config ? " -ORBSvcConf mcast.conf " : '')
      . $bit_off_conf;
}
elsif ($use_reliable_multicast == 1) {
  $svc_config = ($use_svc_config ? " -ORBSvcConf reliable_multicast.conf "
                 : '') . $bit_off_conf;
}

# test multiple cases
$sub_parameters = "$svc_config -u $use_udp -c $use_mcast -s $sub_addr -p $pub_addr -r $num_readers -t $use_take"
              . " -m $num_instances_per_writer -i $num_samples_per_instance"
	      . " -w $num_writers -z $sequence_length"
              . " -k $no_key -y $read_interval_ms -f $mixed_trans"
              . " -a $use_reliable_multicast";

$pub_parameters = "$svc_config -u $use_udp -c $use_mcast -p $pub_addr -w $num_writers "
              . " -m $num_instances_per_writer -i $num_samples_per_instance "
	      . " -n $max_samples_per_instance -z $sequence_length"
	      . " -k $no_key -y $write_interval_ms -b $writer_blocking_ms"
              . " -f $mixed_trans"
              . " -a $use_reliable_multicast";

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
