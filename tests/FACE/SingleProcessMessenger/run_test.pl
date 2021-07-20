eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path("Idl");

my $test = new PerlDDS::TestFramework();

my $config = 'face_config.ini';

if($test->flag('static')) {
    $config = 'face_config_static.ini';
}

my $callback = '';

if($test->flag('callback')) {
    $callback = 'callback';
}

$test->enable_console_logging();

$test->process('SingleProcess', 'SingleProcess/singleprocess', "$config $callback");
$test->start_process('SingleProcess');

exit $test->finish(30);
