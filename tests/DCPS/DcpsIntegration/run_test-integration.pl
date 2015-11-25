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

my $test = new PerlDDS::TestFramework();

my $params = '-DCPSConfigFile infrastructure.ini';

$test->process('test', 'infrastructure_test', $params);
$test->setup_discovery();
$test->start_process('test');
$test->ignore_error('DomainParticipantFactoryImpl::delete_participant, Nil participant');
exit $test->finish(60);
