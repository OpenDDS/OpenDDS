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
        DCPSDefaultDiscovery => 'DEFAULT_REPO',
        DCPSGlobalTransportConfig => '$file'
      },
      'transport/tcp' => {
        transport_type => 'tcp'
      }
    }
  },
  rtps_rtps => {
    discovery => 'rtps',
    file => {
      common => {
        DCPSGlobalTransportConfig => '$file'
      },
      'domain/111' => {
        DiscoveryConfig => 'uni_rtps'
      },
      'rtps_discovery/uni_rtps' => {
        SedpMulticast => '0',
        ResendPeriod => '2'
      },
      'transport/the_rtps_transport' => {
        transport_type => 'rtps_udp',
        use_multicast => '0'
      }
    }
  }
);

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'rtps_rtps');

# Run at high debug level for additional function coverage.
$test->{dcps_debug_level} = 8;

$test->{add_pending_timeout} = 0;
$test->{add_transport_config} = 0;

$test->report_unused_flags(1);
$test->setup_discovery();

my ($rtps_mon) = ($test->{discovery} eq 'rtps') ?
    ('-t 0') : ('');

$test->process("subscriber", "subscriber");
$test->process("publisher", "publisher");
$test->process("monitor1", "monitor", "-l 7 $rtps_mon");
$test->process("monitor2", "monitor", "-u $rtps_mon");

$test->add_temporary_file("monitor1", "monitor1_done");
$test->add_temporary_file("monitor2", "monitor2_done");

$test->start_process("monitor1", "-T");
$test->start_process("publisher", "-T");
$test->start_process("subscriber", "-T");
$test->start_process("monitor2", "-T");

my $status = $test->finish(180, "monitor1");

exit $status;
