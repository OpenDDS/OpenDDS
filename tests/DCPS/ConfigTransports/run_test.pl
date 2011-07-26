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
use Getopt::Std;
use Data::Dumper;

use vars qw/ $opt_d /;
getopts('d:');
my $debug = 1 if $opt_d;



PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

my $pub_time = 5;
my $pub_addr = "localhost:";
my $port=29804;

my $sub_time = $pub_time;
my $sub_addr = "localhost:16701";
#my $use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');
my $use_svc_conf = undef;

my $svc_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf " : '';
my $repo_bit_conf = $use_svc_conf ? "-ORBSvcConf ../../tcp.conf" : '';

my $dcpsrepo_ior = "repo.ior";

my $qos = {
    autoenable    => undef,
    durability => transient_local,
    liveliness => automatic,
    lease_time => 5,
    reliability => reliable,
#    reliability => best_effort,
};

## Entity: [none, participant, pubsub, rw] Determines the level at which
##                  transport configuration is applied.
##          none        - no transport configuration is bound to an entity;
##          participant - the named configuration is bound to domain participant;
##          pubsub      - the named configuration is bound to publisher/sunscriber;
##          rw          - the named configuration is bound to reader/writer. Note
##                      that for this to really take effect, autoenable QoS must be
##                      set to 'false'.
## Configuration: [undef, NAME] Determines which configuration should be applied,
##                  if any.
##          undef       - no transport configuration (*.ini) file is used. Transports
##                      would be expected to have their default names;
##          NAME        - the ./transports.ini will have a [config/NAME]section listing
##                      the allowed transports;
## Collocation: [none, process, participant, in-pubsub] Determines the mutual
##                  distribution of the reader/writer entities.
##          none        - a single reader/writer entity is created by each
##                      subscriber/publisher executable;
##          process     - a reader and a writer is created per executable, using
##                      independent domain participant instances;
##          participant - a reader and a writer entity is created per executable,
##                      using the same domain participant instance, but separate
##                      subscriber/publisher;
##          pubsub      - two readers and two writers are created per executable,
##                      using the same subscriber/publisher instance (same domain
##                      participant instance). The configuration (if any) and the
##                      effective transport assertions are applied to the first
##                      entity instance, only. The second one is asserted to have
##                      the same effective set of transports as its parent (if any,
##                      otherwise - the defaults);
## Protocol:        A list of transport names for the reader/writer entities,
##                  for which the test asserts are being supported. By default,
##                  OpenDDS ships with _OPENDDS_0300_UDP, _OPENDDS_0410_MCAST_UNRELIABLE,
##                  _OPENDDS_0420_MCAST_RELIABLE, _OPENDDS_0500_TCP.
## Compatibility: [true, false] - Whether to assert that the reader and the writer
##           have had their publications matched (via the info repo)
## QoS:             A set of quality of service options. Each executable can get
##                  its own map. QoSes are set before creating the corresponding
##                  entity and before asserting anything about the transports.


my @explicit_configuration = (

  {
    entity        => none,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => [_OPENDDS_0300_UDP, _OPENDDS_0410_MCAST_UNRELIABLE, _OPENDDS_0420_MCAST_RELIABLE, _OPENDDS_0500_TCP],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => participant,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => [udp1],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => pubsub,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => [udp1],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    # Note that without disabling the 'autoenable' policy, the new RW will kick off the
    # transport negotiation and a transport will be selected *before* one has the chance
    # to specify which transport configuration must be used.
    autoenable    => false,
    entity        => rw,
    collocation   => none,
    configuration => Udp_Only,
    protocol      => [udp1],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

);

my @configuration_file_unused = (

  # The effective default configuration contains a transport of the same type,
  # for each transport that was mentioned in the file. The names of the transports
  # are not taken from the configuration file. Any other entity value will cause
  # the test to fail because assigning non-existent configuration to an entity
  # is wrong.
  {
    entity        => none,
    collocation   => none,
    configuration => whatever_just_to_ensure_there_is_a_config_file_on_the_command_line,
    protocol      => [_OPENDDS_0300_UDP, _OPENDDS_0410_MCAST_UNRELIABLE, _OPENDDS_0420_MCAST_RELIABLE, _OPENDDS_0500_TCP],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },
);

my @without_configuration_file = (

  # The effective default configuration in the absence of configuration files
  # contains just the TCP. Any other entity value will have the same effect,
  # because there is no configuration to assign anyway..
  {
    entity        => none,
    collocation   => none,
    configuration => undef,
    protocol      => [_OPENDDS_0500_TCP],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos,
  },
);

my @explicit_configuration_collocated = (

  {
    entity        => none,
    collocation   => process,
    configuration => Udp_Only,
    protocol      => [_OPENDDS_0300_UDP, _OPENDDS_0410_MCAST_UNRELIABLE, _OPENDDS_0420_MCAST_RELIABLE, _OPENDDS_0500_TCP],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => participant,
    collocation   => process,
    configuration => Udp_Only,
    protocol      => [udp1],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    entity        => pubsub,
    collocation   => process,
    configuration => Udp_Only,
    protocol      => [udp1],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

  {
    # Note that without disabling the 'autoenable' policy, the new RW will kick off the
    # transport negotiation and a transport will be selected *before* one has the chance
    # to specify which transport configuration must be used.
    autoenable    => false,
    entity        => rw,
    collocation   => process,
    configuration => Udp_Only,
    protocol      => [udp1],
    compatibility => true,
    publisher     => $qos,
    subscriber    => $qos
  },

);



@scenario = (

  @explicit_configuration_collocated,
  @without_configuration_file,
  @configuration_file_unused,
  @explicit_configuration,

);


# Returns an array of publisher or subscriber command lines
sub parse($$$) {

  my ($pubsub, $hasbuiltins, $s) = @_;

  my $compatibility = $$s{compatibility};
  my $pub_autoenable = $$s{autoenable} ? " -n " . $$s{autoenable} : "" ;

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
         . " -e " . join (' -e ', $pub_entity)
         . $pub_autoenable
         . " -a " . $pub_collocation
         . " -s " . $pub_configuration
         . " -t " . join (' -t ', @$pub_protocol)
         . " -d " . $pub_durability_kind
         . " -k " . $pub_liveliness_kind
         . " -r " . $pub_reliability_kind
         . " -l " . $pub_lease_time
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

sub run($$$$) {

  my ($Publisher, $pub_time, $Subscriber, $sub_time) = @_;

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

sub command($$$) {

  my ($pub_process, $pub_parameters, $debug) = @_; 

  if ($debug != 0) {
    open(FF1, '>/tmp/$pub_process.gdb');
    print FF1 <<EOF;
break main
run $pub_parameters
EOF
    close(FF1);

    return ('/usr/bin/xterm', '-T $pub_process -e gdb -x /tmp/$pub_process.gdb $pub_process');
  }

    return ($pub_process, $pub_parameters);
}



my $count = 0;
my $failed = 0;

for my $hasbuiltins (undef, true) {
#for my $hasbuiltins (undef) {
#for my $hasbuiltins (true) {

    my $DCPSREPO = initialize($hasbuiltins);

    for my $i (@scenario) {

        if ($debug) {
          # You've got an hour ...
          $pub_time = $sub_time = 3600;
        }

        my $status = 0;

        my $pub_parameters = parse('publisher', $hasbuiltins, \%$i);
        my ($process, $parameters) = command('publisher', $pub_parameters, $debug);
        my $S = PerlDDS::create_process ($process, '-x ' . $pub_time . ' ' . $parameters);

        my $sub_parameters = parse('subscriber', $hasbuiltins, \%$i);
        my ($process, $parameters) = command('subscriber', $sub_parameters, $debug);
        my $P = PerlDDS::create_process ($process, '-x ' . $pub_time . ' ' . $parameters);

        if (0 != run($P, $pub_time, $S, $sub_time)) {
          $status++;
        }

        $count++;
        print "count->$count\nstatus->$status\nfailed->$failed\n";

        # Which test failed, exactly?
        if (0 != $status) {
            $failed++;
            $Data::Dumper::Terse = 1;
            print "Test FAILED " . Dumper(\%$i) . "\n";
        }

    }

    if (0 != finalize($DCPSREPO)) {
      $failed++; 
    }
}


if ($failed != 0) {
  print STDERR "FAILED: $failed of $count\n";
}
else {
  print "PASSED: $count\n";
}

exit $failed;

