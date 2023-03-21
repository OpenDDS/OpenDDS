eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $test = new PerlDDS::TestFramework();
$test->process('test', 'TypeSupportPluginUser', join(' ', @ARGV));
$test->start_process('test');
exit $test->finish(60);
