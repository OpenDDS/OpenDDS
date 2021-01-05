eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();

if ($test->flag('durable')) {
  $test->process('rtps_reliability', 'rtps_reliability_durable');
} else {
  $test->process('rtps_reliability', 'rtps_reliability');
}
$test->start_process('rtps_reliability');
my $result = $test->finish (60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
  exit 1;
}

exit 0;
