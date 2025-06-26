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

my %configs = (
    ir_tcp => {
        discovery => 'info_repo',
        file => {
            common => {
                DCPSDefaultDiscovery => 'DEFAULT_REPO',
                DCPSBit => '0'
            },
            'transport/t1' => {
                transport_type => 'tcp',
                max_samples_per_packet => '1'
            },
            'config/c1' => {
                transports => 't1'
            },
            'transport/t2' => {
                transport_type => 'tcp',
                max_samples_per_packet => '1'
            },
            'config/c2' => {
                transports => 't2'
            },
            'transport/t3' => {
                transport_type => 'tcp',
                max_samples_per_packet => '1'
            },
            'config/c3' => {
                transports => 't3'
            },
        }
    },
    rtps_disc => {
        discovery => 'rtps',
        file => {
            common => {
                DCPSGlobalTransportConfig => '$file'
            },
            'domain/23' => {
                DiscoveryConfig => 'fast_rtps'
            },
            'rtps_discovery/fast_rtps' => {
                SedpMulticast => '0',
                ResendPeriod => '2'
            },
            'transport/the_rtps_transport1' => {
                transport_type => 'rtps_udp',
                use_multicast => '0'
            },
            'transport/the_rtps_transport2' => {
                transport_type => 'rtps_udp',
                use_multicast => '0'
            },
            'transport/the_rtps_transport3' => {
                transport_type => 'rtps_udp',
                use_multicast => '0'
            },
            'config/c1' => {
                transports => 'the_rtps_transport1'
            },
            'config/c2' => {
                transports => 'the_rtps_transport2'
            },
            'config/c3' => {
                transports => 'the_rtps_transport3'
            },
            'config/using_rtps' => {
                transports => 'the_rtps_transport3'
            }
        }
    }
);

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'ir_tcp');

my $opts = '';
my $dynamic = 0;
my $nopub = 0;

if ($test->flag('nopub')) {
    $nopub = 1;
    $opts .= " -DCPSPublisherContentFilter 0";
}
if ($test->flag('dynamic')) {
    $dynamic = 1;
}

if ($dynamic) {
  $opts .= ' -dynamic-' . ($nopub ? 'reader' : 'writer');
}

$test->{nobits} = 1;
$test->{add_transport_config} = 0;
$test->setup_discovery();
$test->process('pubsub', 'ContentFilteredTopicTest', $opts);
$test->start_process('pubsub');

exit $test->finish(60);
