eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../FooType');
PerlDDS::add_lib_path('../TestFramework');

my $test = new PerlDDS::TestFramework();
$test->setup_discovery();

$test->process('test', 'test', "@ARGV");
$test->start_process('test');

exit $test->finish(300);
