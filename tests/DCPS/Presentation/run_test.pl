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

PerlDDS::add_lib_path('../FooType');

my %configs = (
    'ir_tcp' => {
        'discovery' => 'info_repo',
        'file' => {
            'common' => {
                'DCPSGlobalTransportConfig' => 'tcp'
            },
            'transport/tcp' => {
                'transport_type' => 'tcp'
            },
            'config/tcp' => {
                'transports' => 'tcp'
            },
        }
    }
);

my $test_opts = "@ARGV";

my $test = new PerlDDS::TestFramework(configs => \%configs, config => 'ir_tcp');

$test->setup_discovery();

$test->process('test', 'test');
$test->start_process('test', "$test_opts");

my $result = $test->finish(180);

exit $result;
