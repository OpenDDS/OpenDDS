eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use Getopt::Std;
use Data::Dumper;
use strict;

use vars qw/ $opt_d $opt_t /;
getopts('d:t:');
my $debug = $opt_d;

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

# An hour should be enough of a timeout for debugging ...
my $pub_time = $opt_t || ($debug ? 3600 : 30);
my $pub_addr = "localhost:";
my $port=29804;

my $sub_time = $pub_time;
my $sub_addr = "localhost:16701";

my $qos = {
    autoenable    => undef,
    durability => 'transient_local',
    liveliness => 'automatic',
    lease_time => 5,
#    reliability => 'reliable',
    reliability => 'best_effort',
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
    entity        => 'none',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['_OPENDDS_0300_UDP', '_OPENDDS_0410_MCAST_UNRELIABLE', '_OPENDDS_0420_MCAST_RELIABLE', '_OPENDDS_0500_TCP'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Alexis Aquila'
  },

  {
    entity        => 'participant',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Feidlimid Chi'
  },

  {
    entity        => 'pubsub',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Nikephoros Romilda'
  },

  {
    # Note that without disabling the 'autoenable' policy, the new RW will kick off the
    # transport negotiation and a transport will be selected *before* one has the chance
    # to specify which transport configuration must be used.
    autoenable    => 'false',
    entity        => 'rw',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Chandra Ananta'
  },

);

my @explicit_configuration_and_negotiation = (

  {
    entity        => 'none',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['_OPENDDS_0300_UDP', '_OPENDDS_0410_MCAST_UNRELIABLE', '_OPENDDS_0420_MCAST_RELIABLE', '_OPENDDS_0500_TCP'],
    compatibility => 'true',
    # The hash referred-to by qos ...
    publisher     => {%$qos, negotiated => ['_OPENDDS_0300_UDP'],},
    subscriber    => {%$qos, negotiated => ['_OPENDDS_0300_UDP'],},
    name          => 'Feidlimid Puck'
  },

    {
    entity        => 'participant',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    negotiated => ['udp1'],
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Vijaya Puck'
  },

  {
    entity        => 'participant',
    collocation   => 'none',
    configuration => 'Tcp_Only',
    protocol      => ['mytcp1'],
    compatibility => 'true',
    negotiated => ['mytcp1'],
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Kamala Fedlimid'
  },

  {
    entity        => 'pubsub',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    negotiated => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Bala Fedlimid'
  },

  {
    entity        => 'pubsub',
    collocation   => 'none',
    configuration => 'Tcp_Only',
    protocol      => ['mytcp1'],
    negotiated => ['mytcp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Padma Columba'
  },

  {
    # Note that without disabling the 'autoenable' policy, the new RW will kick off the
    # transport negotiation and a transport will be selected *before* one has the chance
    # to specify which transport configuration must be used.
    autoenable    => 'false',
    entity        => 'rw',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    negotiated => ['udp1'],
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Padma Feidlimid'
  },

 {
    # Note that without disabling the 'autoenable' policy, the new RW will kick off the
    # transport negotiation and a transport will be selected *before* one has the chance
    # to specify which transport configuration must be used.
    autoenable    => 'false',
    entity        => 'rw',
    collocation   => 'none',
    configuration => 'Tcp_Only',
    protocol      => ['mytcp1'],
    compatibility => 'true',
    negotiated => ['mytcp1'],
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Lalita Kamala'
  },


);

##print "Test " . Dumper(\@explicit_configuration_and_negotiation) . "\n";

my @configuration_file_unused = (

  # The effective default configuration contains a transport of the same type,
  # for each transport that was mentioned in the file. The names of the transports
  # are not taken from the configuration file. Any other entity value will cause
  # the test to fail because assigning non-existent configuration to an entity
  # is wrong.
  {
    entity        => 'none',
    collocation   => 'none',
    configuration => 'whatever_just_to_ensure_there_is_a_config_file_on_the_command_line',
    protocol      => ['_OPENDDS_0300_UDP', '_OPENDDS_0410_MCAST_UNRELIABLE', '_OPENDDS_0420_MCAST_RELIABLE', '_OPENDDS_0500_TCP'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Fedlimid Puck'
  },
);

my @without_configuration_file = (

  # The effective default configuration in the absence of configuration files
  # contains just the TCP. Any other entity value will have the same effect,
  # because there is no configuration to assign anyway..
  {
    entity        => 'none',
    collocation   => 'none',
    configuration => undef,
    protocol      => ['_OPENDDS_0500_TCP'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Ananta Chi'
  },
);

my @explicit_configuration_collocated = (

  {
    entity        => 'none',
    collocation   => 'process',
    configuration => 'Udp_Only',
    protocol      => ['_OPENDDS_0300_UDP', '_OPENDDS_0410_MCAST_UNRELIABLE', '_OPENDDS_0420_MCAST_RELIABLE', '_OPENDDS_0500_TCP'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Alexis Columba'
  },

  {
    entity        => 'participant',
    collocation   => 'process',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Puck Alexis'
  },

  {
    entity        => 'pubsub',
    collocation   => 'process',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Puck Aeron'
  },

  {
    # Note that without disabling the 'autoenable' policy, the new RW will kick off the
    # transport negotiation and a transport will be selected *before* one has the chance
    # to specify which transport configuration must be used.
    autoenable    => 'false',
    entity        => 'rw',
    collocation   => 'process',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Agrippa Fedelmid'
  },

);


my @test_configuration = (


{
    entity        => 'participant',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['udp1'],
    compatibility => 'true',
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Aeron Chi'
  },

{
    entity        => 'participant',
    collocation   => 'none',
    configuration => 'Tcp_Only',
    protocol      => ['mytcp1'],
    compatibility => 'true',
    negotiated => ['mytcp1'],
    publisher     => $qos,
    subscriber    => $qos,
    name          => 'Agrippa Nikephoros'
  },

 {
    entity        => 'none',
    collocation   => 'none',
    configuration => 'Udp_Only',
    protocol      => ['_OPENDDS_0300_UDP', '_OPENDDS_0410_MCAST_UNRELIABLE', '_OPENDDS_0420_MCAST_RELIABLE', '_OPENDDS_0500_TCP'],
    compatibility => 'true',
    # The hash referred-to by qos ...
    publisher     => {%$qos, negotiated => ['_OPENDDS_0300_UDP'],},
    subscriber     => {%$qos, negotiated => ['_OPENDDS_0300_UDP'],},
#    subscriber    => {%$qos, negotiated => ['_OPENDDS_0500_TCP'],},
    name          => 'Fedlimid Aeron'
  },

);

my @scenario = (
  @explicit_configuration_collocated,
  @without_configuration_file,
  @configuration_file_unused,
  @explicit_configuration,
#  @test_configuration,
  @explicit_configuration_and_negotiation,

);


# Returns an array of publisher or subscriber command lines
sub parse($$$) {

  my ($pubsub, $hasbuiltins, $s) = @_;

  my $compatibility = $$s{compatibility};
  my $pub_autoenable = $$s{autoenable} ? " -n " . $$s{autoenable} : "" ;

  my $pub_protocol = $$s{$pubsub}{protocol} || $$s{protocol};
  my $pub_protocol_args = " -t " . join (' -t ', @$pub_protocol) if $pub_protocol;

  my $pub_negotiated = $$s{$pubsub}{negotiated} || $$s{negotiated};
  my $pub_negotiated_args = " -f " . join (' -f ', @$pub_negotiated) if $pub_negotiated;

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
  my $result = "$pub_builtins $level $config"
         . " -c " . $compatibility
         . " -e " . join (' -e ', $pub_entity)
         . $pub_autoenable
         . " -a " . $pub_collocation
         . " -s " . $pub_configuration
         . $pub_protocol_args
         . $pub_negotiated_args
         . " -d " . $pub_durability_kind
         . " -k " . $pub_liveliness_kind
         . " -r " . $pub_reliability_kind
         . " -l " . $pub_lease_time
         ;

  return $result;
}

# This was retained for debugging.
# sub command($$$$) {

#   my ($pub_process, $pub_parameters, $pub_time, $debug) = @_;

#   if ($debug eq 'nemiver') {
#     return ('/usr/bin/nemiver', $pub_process . ' ' . $pub_parameters);
#   }

#   if ($debug eq 'gdb') {
#     my $gdbrc = "/tmp/$pub_process.gdb";
#     open(FF1, ">$gdbrc");
#     print FF1 <<EOF;
# break main
# run $pub_parameters -x $pub_time
# EOF
#     close(FF1);

#     return ('/usr/bin/xterm', '-T ' . $pub_process . ' -e /usr/bin/gdb -x ' . $gdbrc . ' ' . $pub_process);
#   }

#     return ($pub_process, $pub_parameters . ' -x ' . $pub_time);
# }



my $count = 0;
my $failed = 0;

# Only run with the built-in topics when not debugging
my @builtinscases = $debug ? (undef) : (undef, 'true');

for my $hasbuiltins (@builtinscases) {

    for my $i (@scenario) {
        my $test = new PerlDDS::TestFramework();
        $test->enable_console_logging();
        $test->report_unused_flags();

        # Setup the InfoRepo.
        my $info_params = "-NOBITS" unless $hasbuiltins;
        $test->setup_discovery($info_params);

        my $sub_parameters = parse('subscriber', $hasbuiltins, \%$i) . " -x $sub_time";
        $test->process("subscriber", "subscriber", $sub_parameters);

        my $pub_parameters = parse('publisher', $hasbuiltins, \%$i) . " -x $pub_time";
        $test->process("publisher", "publisher", $pub_parameters);

        # What if start fails?
        $test->start_process("subscriber");
        $test->start_process("publisher");
        my $status = $test->finish(($pub_time + 30) * 2, "publisher");

        $count++;
        print "count->$count\nstatus->$status\nfailed->$failed\n";

        # Which test failed, exactly?
        if (0 != $status) {
            $failed++;
            $Data::Dumper::Terse = 1;
            print "Test FAILED (hasbuiltins=" . $hasbuiltins . ")" . Dumper(\%$i) . "\n";
            exit(-1);
        }
    }
}


if ($failed != 0) {
  print STDERR "FAILED: $failed of $count\n";
}
else {
  print "PASSED: $count\n";
}

exit $failed;
