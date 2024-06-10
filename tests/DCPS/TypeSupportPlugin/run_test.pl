eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();
$test->process('test', 'TypeSupportPlugin', join(' ', @ARGV));
$test->start_process('test');
exit $test->finish(60);
