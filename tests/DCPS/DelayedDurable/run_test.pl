eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $cfg = '-DCPSConfigFile rtps_disc.ini';

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();

$test->process('writer', 'DelayedDurable', "$cfg -writer");
$test->start_process('writer');

sleep 15;

$test->process('reader', 'DelayedDurable', "$cfg -reader");
$test->start_process('reader');

exit $test->finish(60);
