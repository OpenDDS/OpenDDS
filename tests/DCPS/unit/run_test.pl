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

my $test = new PerlDDS::TestFramework();
$test->setup_discovery();

# LM_ERROR is expected in the log due to unit tests
my $logFile = 'test.log';
$test->{'report_errors_in_log_file'} = 0;

$test->process('ut', 'DdsDcps_UnitTest', "-ORBLogFile $logFile");
$test->start_process('ut');
exit $test->finish(30);
