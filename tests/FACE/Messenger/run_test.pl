eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

if ($^O ne 'MSWin32' &&
    (new PerlACE::ConfigList)->check_config('OPENDDS_SAFETY_PROFILE') &&
    (new PerlACE::ConfigList)->check_config('SAFETY_BASE')) {
    system('$DDS_ROOT/tools/scripts/analyze_operator_new.sh $ACE_ROOT/lib/libACE.so $DDS_ROOT/lib/libOpenDDS_Corba.so $DDS_ROOT/lib/libOpenDDS_Dcps.so $DDS_ROOT/lib/libOpenDDS_Rtps.so $DDS_ROOT/lib/libOpenDDS_Rtps_Udp.so $DDS_ROOT/lib/libOpenDDS_FACE.so Idl/libFaceMessengerIdl.so Publisher/publisher Subscriber/subscriber | awk \'{ print "ERROR: Call to global operator new: " $2; }\'');
}

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

$test->process('Subscriber', 'Subscriber/subscriber', "$config $callback");
$test->start_process('Subscriber');

sleep 5;

$test->process('Publisher', 'Publisher/publisher', "$config");
$test->start_process('Publisher');
exit $test->finish(30);
