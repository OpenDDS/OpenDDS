eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

push(@ARGV, 'rtps_disc');
my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();
$test->setup_discovery();
$test->process('test', 'UnionTopic');
$test->start_process('test');
exit $test->finish(30);
