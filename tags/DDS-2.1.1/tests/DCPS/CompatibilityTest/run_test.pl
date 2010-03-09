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
# single reader with single instances test
$sub_addr = "localhost:16701";
$pub_addr = "localhost:";
$port=29804;
$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');
$svc_conf = $use_svc_conf ? " -ORBSvcConf ../../tcp.conf " : '';

$dcpsrepo_ior = "repo.ior";
$repo_bit_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf" : '';

unlink $dcpsrepo_ior; 

sub run_compatibility_tests {
  # test multiple cases
  $compatibility = shift;

  $pub_durability_kind = shift;
  $pub_liveliness_kind = shift;
  $pub_lease_time = shift;
  $pub_reliability_kind = shift;

  $sub_durability_kind = shift;
  $sub_liveliness_kind = shift;
  $sub_lease_time = shift;
  $sub_reliability_kind = shift;

  print "\n\n"; # Provide some visual relief between test cases.

  $sub_time = 5;
  $pub_time = $sub_time;
  $sub_parameters = "$svc_conf -s $sub_addr -l $sub_lease_time -x $sub_time -c $compatibility -d $sub_durability_kind -k $sub_liveliness_kind -r $sub_reliability_kind -ORBDebugLevel $level";

  $Subscriber = PerlDDS::create_process ("subscriber", $sub_parameters);

  $pub_parameters = "$svc_conf -c $compatibility -d $pub_durability_kind -k $pub_liveliness_kind -r $pub_reliability_kind -l $pub_lease_time -x $pub_time -ORBDebugLevel $level -p $pub_addr$port" ;

  $Publisher = PerlDDS::create_process ("publisher", "$pub_parameters");

  print $Subscriber->CommandLine() . "\n";
  $SubscriberResult = $Subscriber->Spawn();
  print "Subscriber PID: " . $Subscriber->{PROCESS} . "\n" if $Subscriber->{PROCESS};

  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
  }

  sleep 3;

  print $Publisher->CommandLine() . "\n";
  $Publisher->SpawnWaitKill ($pub_time + 20);
  print "Publisher PID: " . $Publisher->{PROCESS} . "\n" if $Publisher->{PROCESS};

  if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
  }

  $SubscriberResult = $Subscriber->WaitKill($sub_time);

  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
  }

}

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                                         "$repo_bit_conf "
#                           . "-ORBDebugLevel 1 "
                                                   . "-o $dcpsrepo_ior ");
print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
print "Repository PID: " . $DCPSREPO->{PROCESS} . "\n" if $DCPSREPO->{PROCESS};

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

run_compatibility_tests("true", "transient_local", "automatic", "5", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("true", "transient_local", "automatic", "6", "reliable", "volatile", "automatic", "7", "best_effort");
run_compatibility_tests("true", "transient_local", "automatic", "5", "reliable", "volatile", "automatic", "infinite", "best_effort");
run_compatibility_tests("true", "transient_local", "automatic", "infinite", "reliable", "volatile", "automatic", "infinite", "best_effort");

run_compatibility_tests("false", "transient_local", "automatic", "6", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "transient_local", "automatic", "infinite", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "transient_local", "automatic", "6", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "transient_local", "automatic", "5", "best_effort", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "volatile", "automatic", "5", "reliable", "transient_local", "automatic", "5", "reliable");
# there are more to test later, but they are currently treated as invalid
# since there are not supported in the code


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
