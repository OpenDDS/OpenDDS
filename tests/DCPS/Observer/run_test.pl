eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');
PerlDDS::add_lib_path('../common');

my $test = new PerlDDS::TestFramework();

$test->process('publisher', 'publisher', " -DCPSConfigFile rtps_disc.ini");
$test->process('subscriber', 'subscriber', " -DCPSConfigFile rtps_disc.ini");

$test->start_process('subscriber');
$test->start_process('publisher');

my $result = $test->finish(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
}

exit $result;
