eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();
$test->process('rt', 'RestartTest', @ARGV);
$test->start_process('rt');
exit $test->finish(60);
