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

my $parameters = '-DCPSBit 0';

if ($ARGV[0] eq 'by_instance') {
  $parameters .= ' -i';
}

my $test = new PerlDDS::TestFramework();
$test->setup_discovery('-NOBITS');
$test->enable_console_logging();

$test->process('main', 'main', $parameters);
$test->start_process('main');

exit $test->finish(60);
