eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');

my %configs = (
  ir_tcp => {
    discovery => 'info_repo',
    file => {
      common => {
        pool_size => '900000000',
        DCPSGlobalTransportConfig => 'myconfig'
      },
      'domain/42' => {
        DiscoveryConfig => 'repo'
      },
      'repository/repo' => {
        RepositoryIor => 'file://repo.ior'
      },
      'config/myconfig' => {
        transports => 'the_tcp_transport'
      },
      'transport/the_tcp_transport' => {
        transport_type => 'tcp'
      }
    }
  },
  rtps => {
    discovery => 'rtps',
    file => {
      common => {
        pool_size => '900000000',
        DCPSGlobalTransportConfig => 'myconfig'
      },
      'domain/42' => {
        DiscoveryConfig => 'uni_rtps'
      },
      'rtps_discovery/uni_rtps' => {
        SedpMulticast => '0',
        ResendPeriod => '2',
        SedpPassiveConnectDuration => '900000',
        MaxSpdpSequenceMsgResetChecks => '10000'
      },
      'config/myconfig' => {
        transports => 'the_rtps_transport',
        passive_connect_duration => '900000'
      },
      'transport/the_rtps_transport' => {
        transport_type => 'rtps_udp',
        use_multicast => '0',
        nak_depth => '512',
        heartbeat_period => '200'
      }
    }
  }
);

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'ir_tcp');
$test->{add_transport_config} = 0;

my $debug_level = 0;
my $opts = "";
# $opts .= "-DCPSChunks 1 ";

if ($test->flag('low')) {
  $opts .= "-t 8 -s 128 -n 1024";
} elsif ($test->flag('medium')) {
  $opts .= "-t 16 -s 64 -n 1024";
} elsif ($test->flag('high')) {
  $opts .= "-t 32 -s 32 -n 1024";
} elsif ($test->flag('aggressive')) {
  $opts .= "-t 64 -s 16 -n 1024";
} elsif ($test->flag('triangle')) {
  $opts .= "-t 3 -s 3 -n 9";
} elsif ($test->flag('double')) {
  $opts .= "-t 2 -s 1 -n 2";
} elsif ($test->flag('single')) {
  $opts .= "-t 1 -s 1 -n 1";
} elsif ($test->flag('superlow')) {
  $opts .= "-t 4 -s 256 -n 1024";
} elsif ($test->flag('megalow')) {
  $opts .= "-t 2 -s 512 -n 1024";
} else { # default (i.e. lazy)
  $opts .= "-t 1 -s 1024 -n 1024";
}

if ($test->flag('durable')) {
  $opts .= " -d";
}

if ($debug_level) {
  unlink 'Thrasher.log';
  $opts .= " -DCPSDebugLevel $debug_level -DCPSTransportDebugLevel $debug_level -ORBLogFile Thrasher.log";
}

$test->setup_discovery();
$test->enable_console_logging();
$test->process('Thrasher', 'Thrasher', $opts);
$test->start_process('Thrasher');

exit $test->finish(900);
