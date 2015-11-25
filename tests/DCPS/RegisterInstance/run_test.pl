eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

my $options = '';
my $DCPSREPO;

# The test results in error output from attempting to register
# more instances than allowed. Capture it to prevent false alarms.
my $logfile = 'output.log';

unlink $logfile;

PerlDDS::add_lib_path('../FooType');

$options = "-DCPSConfigFile rtps_disc.ini -ORBLogFile $logfile";

my $test = new PerlDDS::TestFramework();
$test->ignore_error("register instance with container failed");
$test->ignore_error("register failed");

$test->process("register_instance_test", "register_instance_test", "$options");
$test->start_process("register_instance_test");
$result = $test->finish(60);

if ($result != 0) {
    print STDERR "ERROR: test returned $result\n";
    $status = 1;
}

exit $status;
