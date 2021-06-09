eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $name = 'Compiler_Typecode_C++11';
my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();
$test->process('tc', 'Compiler_Typecode_C++11');
$test->start_process('tc');
exit $test->finish(15);
