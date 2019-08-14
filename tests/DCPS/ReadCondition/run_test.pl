eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();
$test->{'nobits'} = 1;

$test->setup_discovery();
$test->process('rc', 'ReadConditionTest', '-DCPSConfigFile dcps.ini');
$test->start_process('rc');

exit $test->finish(60);
