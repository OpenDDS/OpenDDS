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

my $status = 0;

PerlDDS::add_lib_path('../common');

my $test = new PerlDDS::TestFramework();

$test->process('proc1', 'publisher', '-p1 -p2 -s6');
$test->process('proc2', 'publisher', '-p3 -p4 -p5 -s7');
$test->process('proc3', 'subscriber', '-s1 -s2 -s3 -s4 -s5 -p6 -p7');

for my $n (1..7) {
  $test->add_temporary_file('proc1', "T${n}_publisher_finished.txt");
  $test->add_temporary_file('proc3', "T${n}_subscriber_finished.txt");
}

$test->setup_discovery();

$test->start_process('proc1', '-T');
$test->start_process('proc2', '-T');
$test->start_process('proc3', '-T');

exit $test->finish(300);
