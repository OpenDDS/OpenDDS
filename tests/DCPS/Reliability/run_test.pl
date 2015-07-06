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

PerlDDS::add_lib_path('./IDL');

my $num_sleeps = 5;
my $sleep_secs = 2;
# Use double the sleep time to ensure reader waits long
# enough to see activity after a forced sleep to register
# still active and continue reading
my $max_timeout = 2*$sleep_secs;

my $pub_opts = " -DCPSPendingTimeout $max_timeout";
my $sub_opts = " -num_sleeps $num_sleeps -sleep_secs $sleep_secs -DCPSPendingTimeout $max_timeout";

my $test = new PerlDDS::TestFramework();

if ($test->flag('take-next')) {
  $sub_opts .= " -take-next";
}
elsif ($test->flag('take')) {
  $sub_opts .= " -take";
}
elsif ($test->flag('zero-copy')) {
  $sub_opts .= " -zero-copy";
}
if ($test->flag('rtps')) {
  $pub_opts .= " 50";
}
if ($test->flag('keep-last-one')) {
  if (!$test->flag('rtps')) {
    $pub_opts .= " 50";
  }
  $pub_opts .= " -keep-last-one";
  $sub_opts .= " -keep-last-one";
}

$test->setup_discovery();
$test->enable_console_logging();

$test->process('sub', 'sub/subscriber', $sub_opts);
$test->process('pub', 'pub/publisher', $pub_opts);

$test->start_process('sub');
$test->start_process('pub');

exit $test->finish(1200);
