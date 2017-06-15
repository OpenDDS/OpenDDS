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

my $test = new PerlDDS::TestFramework();
my $is_rtps_disc = 0;
my $DCPScfg = "";

if ($ARGV[0] eq "rtps_disc") {
  $DCPScfg = "-DCPSConfigFile " . $ARGV[0] . ".ini ";
  $is_rtps_disc = 1;
  shift;
}

$test->process('sub', 'subscriber');
$test->process('pub', 'publisher');

$test->setup_discovery() unless $is_rtps_disc;
$test->start_process('pub');
$test->start_process('sub');

$test->ignore_error('Entity is not enabled.');
exit $test->finish(60);
