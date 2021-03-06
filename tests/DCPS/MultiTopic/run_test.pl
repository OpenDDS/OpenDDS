eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $test = new PerlDDS::TestFramework();

my $is_rtps_disc = $test->{'flags'}->{'rtps_disc'} || 0;

my $dir;
if ($test->{'flags'}->{'cpp11'}) {
    $dir = 'cpp11';
}
if ($test->{'flags'}->{'classic'}) {
    $dir = 'classic';
}

if (! $dir) {
    print "dir not set\n";
    exit 1;
}

my $args;
if ($is_rtps_disc) {
  $args = ' -DCPSConfigFile rtps_disc.ini';
} else {
  $test->setup_discovery();
}

$test->process('proc', "$dir/MultiTopicTest", $args);
$test->start_process('proc');

exit $test->finish(30);
