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

PerlDDS::add_lib_path('../FooType4');

my $test = new PerlDDS::TestFramework();
$test->{add_pending_timeout} = 0;
$test->enable_console_logging();

# default get_builtin_subscriber test
my $ignore_kind=0;

if ($test->flag('ignore_part')) {
  $ignore_kind = 1;
}
elsif ($test->flag('ignore_topic')) {
  $ignore_kind = 2;
}
elsif ($test->flag('ignore_pub')) {
  $ignore_kind = 3;
}
elsif ($test->flag('ignore_sub')) {
  $ignore_kind = 4;
}

$test->report_unused_flags(1);

$test->setup_discovery();

$test->process("client", "bit", " -i $ignore_kind ");
$test->start_process("client");

my $status = $test->finish(60);
exit $status;
