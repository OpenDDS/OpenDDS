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

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');
# single reader with single instances test
$pub_time = 5;
$pub_addr = "localhost:";
$port=29804;
$sub_time = $pub_time;
$sub_addr = "localhost:16701";
#$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');
$use_svc_conf = false;
$svc_conf = $use_svc_conf ? " -ORBSvcConf ../../tcp.conf " : '';
$repo_bit_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf" : '';

$dcpsrepo_ior = "repo.ior";

$qos = {
    durability => transient_local,
    liveliness => automatic,
    lease_time => 5,
    reliability => reliable
};

@scenario = (

# A transport configuration file
  {
    entity        => none,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => _OPENDDS_0300_UDP,
    compatibility => false,
    publisher     => $qos,
    subscriber    => $qos
  },

#); @scenario_other = (

  {
    entity        => pubsub,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => _OPENDDS_0300_UDP,
    compatibility => false,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => participant,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => _OPENDDS_0300_UDP,
    compatibility => false,
    publisher     => $qos,
    subscriber    => $qos
  },

# No transport configuration file
  {
    entity        => none,
    collocation   => none,
    configuration => none,
    protocol      => _OPENDDS_0500_TCP,
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => pubsub,
    collocation   => none,
    configuration => none,
    protocol      => _OPENDDS_0500_TCP,
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => participant,
    collocation   => none,
    configuration => none,
    protocol      => _OPENDDS_0500_TCP,
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

);


# Returns an array of publisher command lines
sub parse($) {

  my ($s) = @_;

  my $compatibility = $$s{compatibility};

  my $pub_protocol = $$s{publisher}{protocol} || $$s{protocol};
  my $pub_entity = $$s{publisher}{entity} || $$s{entity};
  my $pub_collocation = $$s{publisher}{collocation} || $$s{collocation};
  my $pub_configuration = $$s{publisher}{configuration} || $$s{configuration};

  my $pub_durability_kind = $$s{publisher}{durability} || $$s{durability};
  my $pub_liveliness_kind = $$s{publisher}{liveliness} || $$s{liveliness};
  my $pub_lease_time = $$s{publisher}{lease_time} || $$s{lease_time};
  my $pub_reliability_kind = $$s{publisher}{reliability} || $$s{reliability};

  my $sub_protocol = $$s{subscriber}{protocol} || $$s{protocol};
  my $sub_entity = $$s{subscriber}{entity} || $$s{entity};
  my $sub_collocation = $$s{subscriber}{collocation} || $$s{collocation};
  my $sub_configuration = $$s{subscriber}{configuration} || $$s{configuration};

  my $sub_entity = $$s{subscriber}{entity} || $$s{entity};
  my $sub_durability_kind = $$s{subscriber}{durability} || $$s{durability};
  my $sub_liveliness_kind = $$s{subscriber}{liveliness} || $$s{liveliness};
  my $sub_lease_time = $$s{subscriber}{lease_time} || $$s{lease_time};
  my $sub_reliability_kind = $$s{subscriber}{reliability} || $$s{reliability};

  my $level = "-ORBDebugLevel " . $$s{verbosity} if $$s{verbosity};
#  my $common = "$svc_conf -DCPSBIT 0 $level -DCPSConfigFile transports.ini -c $compatibility";
  my $common = "$svc_conf $level -DCPSConfigFile transports.ini -c $compatibility";

  my $pub_parameters = $common
                      . " -e " . $pub_entity
                      . " -a " . $pub_collocation
                      . " -s " . $pub_configuration
                      . " -t " . $pub_protocol
                      . " -d " . $pub_durability_kind
                      . " -k " . $pub_liveliness_kind
                      . " -r " . $pub_reliability_kind
                      . " -l " . $pub_lease_time
                      . " -x " . $pub_time
                      ;

  my $sub_parameters = $common
                      . " -e " . $sub_entity
                      . " -a " . $sub_collocation
                      . " -s " . $sub_configuration
                      . " -t " . $sub_protocol
                      . " -d " . $sub_durability_kind
                      . " -k " . $sub_liveliness_kind
                      . " -r " . $sub_reliability_kind
                      . " -l " . $sub_lease_time
                      . " -x " . $sub_time
                      ;

  return ($pub_parameters, $sub_parameters);
}


sub initialize() {
  unlink $dcpsrepo_ior;

  my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                                         "$repo_bit_conf "
#                             . "-ORBDebugLevel 1 "
#                             . "-NOBITS "
                                                   . "-o $dcpsrepo_ior ");
  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn ();
  print "Repository PID: " . $DCPSREPO->{PROCESS} . "\n" if $DCPSREPO->{PROCESS};

  if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
      print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
      $DCPSREPO->Kill ();
      exit 1;
  }

  return $DCPSREPO;
}

sub finalize($) {
  my $DCPSREPO = shift;
  my $ir = $DCPSREPO->TerminateWaitKill(5);

  unlink $dcpsrepo_ior;

  if ($ir != 0) {
      print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
      return 1;
  }

  return 0;
}

sub run($$) {
  my ($pub_parameters, $sub_parameters) = @_;

  my $Subscriber = PerlDDS::create_process ("subscriber", $sub_parameters);
  my $Publisher = PerlDDS::create_process ("publisher", $pub_parameters);
  my $status = 0;

  print $Subscriber->CommandLine() . "\n";

  my $SubscriberResult = $Subscriber->Spawn();
  print "Subscriber PID: " . $Subscriber->{PROCESS} . "\n" if $Subscriber->{PROCESS};

  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
  }

  sleep 3;

  print $Publisher->CommandLine() . "\n";

  my $PublisherResult = $Publisher->SpawnWaitKill ($pub_time + 20);
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

  return $status;
}


my $DCPSREPO = initialize();
my $status = 0;

for my $i (@scenario) {
    ($pub_parameters, $sub_parameters) = parse(\%$i);
    $status += run($pub_parameters, $sub_parameters);
    print "\n";
}

$status += finalize($DCPSREPO);

if ($status == 0) {
  print "test PASSED\n";
}
else {
  print STDERR "test FAILED: $status\n";
}

exit $status;



#my $DCPSREPO = initialize();

#run_scenario("true", "transient_local", "automatic", "5", "reliable", "transient_local", "automatic", "5", "reliable");
#run_scenario("true", "transient_local", "automatic", "6", "reliable", "volatile", "automatic", "7", "best_effort");
#run_scenario("true", "transient_local", "automatic", "5", "reliable", "volatile", "automatic", "infinite", "best_effort");
#run_scenario("true", "transient_local", "automatic", "infinite", "reliable", "volatile", "automatic", "infinite", "best_effort");
#
#run_scenario("false", "transient_local", "automatic", "6", "reliable", "transient_local", "automatic", "5", "reliable");
#run_scenario("false", "transient_local", "automatic", "infinite", "reliable", "transient_local", "automatic", "5", "reliable");
#run_scenario("false", "transient_local", "automatic", "6", "reliable", "transient_local", "automatic", "5", "reliable");
#run_scenario("false", "transient_local", "automatic", "5", "best_effort", "transient_local", "automatic", "5", "reliable");
#run_scenario("false", "volatile", "automatic", "5", "reliable", "transient_local", "automatic", "5", "reliable");
## there are more to test later, but they are currently treated as invalid
## since there are not supported in the code
#
#my $status = finalize($DCPSREPO);
#exit $status;
