eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
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

my $pub_opts = "";
my $sub_opts = " -num_sleeps $num_sleeps -sleep_secs $sleep_secs -DCPSPendingTimeout $sleep_secs";

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
