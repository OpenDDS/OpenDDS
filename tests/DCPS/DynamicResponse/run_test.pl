eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $test = new PerlDDS::TestFramework();
$test->{dcps_debug_level} = 4;
$test->process('origin', 'origin');
$test->process('responder', 'responder');
$test->start_process('origin');
$test->start_process('responder');
exit $test->finish(60);
