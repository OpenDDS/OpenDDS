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

my $subscriber_completed = "subscriber_finished.txt";
my $subscriber_ready = "subscriber_ready.txt";
my $publisher_ready = "publisher_ready.txt";
my $testoutputfilename = "test.log";

unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_ready;
unlink $testoutputfilename;

my $test = new PerlDDS::TestFramework();
$test->setup_discovery('-ORBSvcConf repo.conf');

$test->enable_console_logging();

$test->process('sub', 'subscriber', '-DCPSConfigFile sub.ini -v');

my $verbose_opt = $test->{'flags'}->{'verbose'} ? '-v ' : '';
$test->process('pub', 'publisher', "-DCPSConfigFile pub.ini $verbose_opt");

$test->start_process('sub');
$test->start_process('pub');

my $result = $test->finish(300);

unlink $subscriber_completed;
unlink $subscriber_ready;
unlink $publisher_ready;
exit $result;
