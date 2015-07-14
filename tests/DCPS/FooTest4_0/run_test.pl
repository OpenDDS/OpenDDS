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

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

my $parameters = "-DCPSLivelinessFactor 300 -z " ;

my $test = new PerlDDS::TestFramework();

$test->setup_discovery();
$test->enable_console_logging();
$test->process('ft40', 'FooTest4_0', $parameters);
$test->start_process('ft40');

exit $test->finish(120);
