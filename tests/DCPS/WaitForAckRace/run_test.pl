eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my %configs = (
  ir_tcp => {
    discovery => 'info_repo',
    file => {
      common => {
        DCPSGlobalTransportConfig => '$file',
        DCPSDefaultDiscovery => 'DEFAULT_REPO'
      },
      'transport/the_tcp_transport' => {
          transport_type => 'tcp'
      }
    },
  },
  rtps => {
    discovery => 'info_repo',
    file => {
      common => {
        DCPSGlobalTransportConfig => '$file',
        DCPSDefaultDiscovery => 'DEFAULT_REPO'
      },
      'transport/the_rtps_transport' => {
        transport_type => 'rtps_udp',
        max_message_size => '1300'
      }
    }
  },
  rtps_disc => {
    discovery => 'rtps',
    file => {
      'common' => {
        DCPSGlobalTransportConfig => '$file',
        DCPSPendingTimeout => '3'
      },
      'domain/31' => {
        DiscoveryConfig => 'uni_rtps'
      },
      'rtps_discovery/uni_rtps' => {
        InteropMulticastOverride => '239.255.1.0',
        SedpMulticast => '1',
        ResendPeriod => '2'
      },
      'transport/the_rtps_transport' => {
        transport_type => 'rtps_udp',
        multicast_group_address => '239.255.2.0',
        max_message_size => '1300'
      }
    }
  },
  rtps_disc_tcp => {
    discovery => 'rtps',
    file => {
      common => {
        DCPSGlobalTransportConfig => '$file'
      },
      'domain/31' => {
        DiscoveryConfig => 'fast_rtps'
      },
      'rtps_discovery/fast_rtps' => {
        SedpMulticast => '0',
        ResendPeriod => '2'
      },
      'transport/t1' => {
        transport_type => 'tcp'
      }
    }
  }
);

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'ir_tcp');
$test->{add_transport_config} = 0;

my $pub_count = 0;
my $sub_count = 0;
my $large_samples = 0;
my $sub_delay = 0;

my $ai = 0;
foreach $a(@ARGV) {
  if ($a eq "large_samples") {
    $large_samples = 1;
  }
  if ($a eq "sub_delay") {
    $sub_delay = 1;
  }
#  if ($a eq "publishers") {
#    $pub_count = @ARGV[$ai + 1];
#  }
  if ($a eq "subscribers") {
    $sub_count = @ARGV[$ai + 1];
  }
  $ai++;
}

$test->{'dcps_debug_level'} = 0;
$test->{'dcps_transport_debug_level'} = 0;
$test->setup_discovery();

if ($pub_count == 0) {
  $pub_count = 1;
}

if ($sub_count == 0) {
  $sub_count = 3;
}

my $pub_index = 1;
my $sub_index = 1;

my $pub_args = "";
my $sub_args = "";

if ($large_samples) {
  $pub_args = $pub_args . " -l";
  $sub_args = $sub_args . " -l";
}
if ($sub_delay) {
  $pub_args = $pub_args . " -d";
  $sub_args = $sub_args . " -d";
}
$pub_args = $pub_args . " -r $sub_count" . " -DCPSPendingTimeout 3 -ORBVerboseLogging 1";

$sub_args = $sub_args . " -DCPSPendingTimeout 3 -ORBVerboseLogging 1";

for (my $i = 0; $i < $pub_count; $i++) {
  $test->process("pub_" . $pub_index++, "publisher", $pub_args);
}

for (my $i = 0; $i < $sub_count; $i++) {
  $test->process("sub_" . $sub_index++, "subscriber", $sub_args);
}

$pub_index = 1;
$sub_index = 1;

for (my $i = 0; $i < $pub_count; $i++) {
  $test->start_process("pub_" . $pub_index++);
}

for (my $i = 0; $i < $sub_count; $i++) {
  sleep(3);
  $test->start_process("sub_" . $sub_index++);
}

exit $test->finish(30);
