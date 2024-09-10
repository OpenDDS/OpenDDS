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
                DCPSBit => '0',
                DCPSInfoRepo => 'file://repo.ior',
                DCPSChunks => '20',
                DCPSChunkAssociationMultiplier => '10',
                DCPSLivelinessFactor => '80',
                DCPSGlobalTransportConfig => '$file'
            },
            'transport/tcp' => {
                transport_type => 'tcp'
            },
        }
    }
);

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'ir_tcp');
$test->setup_discovery('-NOBITS');

$test->process('subscriber1', 'subscriber', '-t5 -n5 -s5 -p10 -a s1 -A p1 -A p2 -A s1');
$test->process('publisher1',  'publisher',  '-t5 -n5 -p5 -s5  -a p1 -A p1 -A p2 -A s1');
$test->process('publisher2',  'publisher',  '-t5 -n5 -p5 -s5  -a p2 -A p1 -A p2 -A s1');

$test->start_process('subscriber1');
$test->start_process('publisher1');
$test->start_process('publisher2');

exit($test->finish(60));
