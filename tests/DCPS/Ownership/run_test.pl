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

my $status = 0;

my $debuglevel = 0;
my $debug_opts = "-ORBDebugLevel $debuglevel -ORBVerboseLogging 1 -DCPSDebugLevel $debuglevel -DCPSTransportDebugLevel $debuglevel";

my $pub_opts = "$debug_opts";
my $sub_opts = "$debug_opts";
my $shutdown_pub = 0;
my $sub_deadline = "";
my $pub1_deadline = "";
my $pub2_deadline = "";
my $pub1_reset_strength = "";
my $sub_liveliness = "";
my $pub1_liveliness = "";
my $pub2_liveliness = "";
my $testcase = 0;

my %configs = (
    ir_tcp => {
        discovery => 'info_repo',
        file => {
            common => {
                DCPSDefaultDiscovery => 'DEFAULT_REPO',
                DCPSDebugLevel => '0',
                DCPSInfoRepo => 'file://repo.ior',
                DCPSChunks => '20',
                DCPSChunkAssociationMultiplier => '10',
                DCPSLivelinessFactor => '80',
                DCPSGlobalTransportConfig => '$file'
            },
            'transport/tcp' => {
                transport_type => 'tcp'
            }
        }
    },
    rtps => {
        discovery => 'rtps',
        file => {
            common => {
                DCPSGlobalTransportConfig => '$file',
                DCPSDebugLevel => '0',
                DCPSChunks => '20',
                DCPSChunkAssociationMultiplier => '10',
                DCPSLivelinessFactor => '80'
            },
            'domain/111' => {
                DiscoveryConfig => 'fast_rtps'
            },
            'rtps_discovery/fast_rtps' => {
                SedpMulticast => '0',
                ResendPeriod => '2'
            },
            'transport/the_rtps_transport1' => {
                transport_type => 'rtps_udp'
            }
        }
    }
);

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'ir_tcp');
$test->{add_transport_config} = 0;
$test->ignore_error('TAO.*Transport.*handle_input, error parsing incoming message');

if ($test->flag('liveliness_change')) {
    $sub_liveliness = "-l 2";
    $pub1_liveliness = "-l 1 -y 250";
    $pub2_liveliness = "-l 1 -y 250 -c";
    $testcase = 1;
} elsif ($test->flag('miss_deadline')) {
    $sub_deadline = "-d 2";
    $pub1_deadline = "-d 1 -y 500";
    $pub2_deadline = "-d 1 -y 500 -c";
    $testcase = 2;
} elsif ($test->flag('update_strength')) {
    $pub1_reset_strength = "-r 15";
    $testcase = 3;
}

$test->setup_discovery("$debug_opts -ORBLogFile DCPSInfoRepo.log");

$test->process('subscriber', 'subscriber', " $sub_opts -ORBLogFile sub.log $sub_deadline $sub_liveliness -t $testcase");
$test->process('publisher1', 'publisher', "$pub_opts -ORBLogFile pub1.log -s 10 -i datawriter1 $pub1_reset_strength $pub1_deadline $pub1_liveliness");
$test->process('publisher2', 'publisher', "$pub_opts -ORBLogFile pub2.log -s 12 -i datawriter2 $pub2_deadline $pub2_liveliness");

$test->start_process('publisher1');
$test->start_process('subscriber');

if ($testcase == 0) {
  sleep (3);
}

$test->start_process('publisher2');

exit $test->finish(60);
