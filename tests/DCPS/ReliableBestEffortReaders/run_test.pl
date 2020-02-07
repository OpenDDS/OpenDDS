eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

sub do_test {
  my $reliable = shift;
  my $test = new PerlDDS::TestFramework();

  $test->process('publisher', 'publisher', " -DCPSConfigFile rtps_disc.ini");
  $test->process('subscriber', 'subscriber', " -DCPSConfigFile rtps_disc.ini -r $reliable");

  $test->start_process('subscriber');
  $test->start_process('publisher');

  my $result = $test->finish(60);
  if ($result != 0) {
    print STDERR "ERROR: test returned $result\n";
  }
  return $result;
}

print "\nTesting best-effort readers:\n";
my $result = do_test("00");

print "\nTesting reliable readers:\n";
$result += do_test("11");

print "\nTesting best-effort and reliable readers:\n";
$result += do_test("01");

print "\nTesting reliable and best-effort readers:\n";
$result += do_test("10");

exit $result;
