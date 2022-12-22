eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use strict;
use warnings;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $test = new PerlDDS::TestFramework();

my $dir;
if ($test->flag('cpp11')) {
  $dir = 'cpp11';
}
elsif ($test->flag('classic')) {
  $dir = 'classic';
}

if (!defined($dir)) {
  die("Requires mapping type");
}

my $args;
if ($test->flag('rtps_disc')) {
  $args = ' -DCPSConfigFile rtps_disc.ini';
}
else {
  $test->setup_discovery();
}

$test->process('proc', "$dir/MultiTopicTest", $args);
$test->start_process('proc');

exit $test->finish(30);
