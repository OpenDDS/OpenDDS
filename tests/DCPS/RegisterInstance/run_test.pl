eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
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

$Topic = PerlDDS::create_process ("register_instance_test", "$options");

print $Topic->CommandLine() . "\n";

$TopicResult = $Topic->SpawnWaitKill (60);

if ($TopicResult != 0) {
    print STDERR "ERROR: topic_test returned $TopicResult\n";
    $status = 1;
}

exit $status;
