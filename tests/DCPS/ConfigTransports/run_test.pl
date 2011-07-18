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
$use_svc_conf = undef;
$svc_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf " : '';
$repo_bit_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf" : '';


$dcpsrepo_ior = "repo.ior";

$qos = {
    durability => transient_local,
    liveliness => automatic,
    lease_time => 5,
    reliability => reliable
};

@scenario = (

# A transport configuration file and using the Udp_Only
# configuration.
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

# No transport configuration is specified
# The defaults (TCP) will be used
  {
    entity        => none,
    collocation   => none,
    configuration => undef,
    protocol      => _OPENDDS_0500_TCP,
    compatibility => false,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => pubsub,
    collocation   => none,
    configuration => undef,
    protocol      => _OPENDDS_0500_TCP,
    compatibility => false,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => participant,
    collocation   => none,
    configuration => undef,
    protocol      => _OPENDDS_0500_TCP,
    compatibility => false,
    publisher     => $qos,
    subscriber    => $qos
  },

);


# Returns an array of publisher or subscriber command lines
sub parse($$$) {

  my ($pubsub, $hasbuiltins, $s) = @_;

  my $compatibility = $$s{compatibility};

  my $pub_protocol = $$s{$pubsub}{protocol} || $$s{protocol};
  my $pub_entity = $$s{$pubsub}{entity} || $$s{entity};
  my $pub_collocation = $$s{$pubsub}{collocation} || $$s{collocation};
  my $pub_configuration = $$s{$pubsub}{configuration} || $$s{configuration};

  my $pub_durability_kind = $$s{$pubsub}{durability} || $$s{durability};
  my $pub_liveliness_kind = $$s{$pubsub}{liveliness} || $$s{liveliness};
  my $pub_lease_time = $$s{$pubsub}{lease_time} || $$s{lease_time};
  my $pub_reliability_kind = $$s{$pubsub}{reliability} || $$s{reliability};

  my $pub_builtins = "-DCPSBIT 0" unless $hasbuiltins;

  my $level = "-ORBDebugLevel " . $$s{verbosity} if $$s{verbosity};
  my $config = "-DCPSConfigFile transports.ini" if $pub_configuration;

  $pub_configuration = $pub_configuration || 'none';
  my $result = "$svc_conf $pub_builtins $level $config" 
         . " -c " . $compatibility
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

  return $result;
}


sub initialize($) {

  my ($hasbuiltins) = @_;

  unlink $dcpsrepo_ior;

  my $pub_builtins = "-NOBITS" unless $hasbuiltins;

  my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                                         "$repo_bit_conf $pub_builtins "
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


my $status = 0;

for my $hasbuiltins (undef, true) {

    my $DCPSREPO = initialize($hasbuiltins);

    for my $i (@scenario) {

	$pub_parameters = parse('publisher', $hasbuiltins, \%$i);
	$sub_parameters = parse('subscriber', $hasbuiltins, \%$i);

	$status += run($pub_parameters, $sub_parameters);
	print "\n";
    }

    $status += finalize($DCPSREPO);
}


if ($status == 0) {
  print "test PASSED\n";
}
else {
  print STDERR "test FAILED: $status\n";
}

exit $status;

