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

my $data_file = "test_run.data";
unlink $data_file;

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();
$test->setup_discovery();

my $late = $test->flag('late');
$test->process('sub', 'subscriber', $late ? '-l 10' : '');
$test->process('pub', 'publisher', ($late ? '-o 10' : '')
               . " -ORBLogFile $data_file");

$test->start_process('pub');

if (PerlACE::waitforfile_timed($data_file, 30) == -1) {
    print STDERR "ERROR: waiting for Publisher file\n";
    $test->finish();
    exit 1;
}

# Sleep for 2 seconds after publisher send all samples to avoid the
# timing issue that the subscriber may start and finish in 1 second
# while the publisher is waiting for it to start.
sleep(2);

$test->start_process('sub');

if (PerlACE::waitforfileoutput_timed($data_file, "Done writing", 90) == -1) {
    print STDERR "ERROR: waiting for Publisher output.\n";
    $test->finish();
    exit 1;
}

exit $test->finish(300);
