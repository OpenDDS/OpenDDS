eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path("$ENV{DDS_ROOT}/tests/DCPS/ConsolidatedMessengerIdl");

my $test = new PerlDDS::TestFramework();

$test->process('CheckInstance', 'CheckInstance', "");
$test->start_process('CheckInstance');
my $result = $test->finish(10);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
  exit 1;
}

exit 0;
