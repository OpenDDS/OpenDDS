eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();
$test->{dcps_debug_level} = 8;

my $opts .= "-ORBDebugLevel 8 -DCPSConfigFile rtps_disc.ini";
$test->process('max_in_authentication', 'max_in_authentication', $opts);
$test->start_process('max_in_authentication');
my $result = $test->finish(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
  exit 1;
}

exit 0;
