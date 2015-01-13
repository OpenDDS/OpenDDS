eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

# Force to number.
my $original_d0 = $ENV{'OPENDDS_RTPS_DEFAULT_D0'} + 0;

my $result = 0;
my @configs = $PerlDDS::SafetyProfile ? qw/rtps_disc.ini rtps_disc_group.ini/ :
    qw/rtps_disc.ini rtps_disc_tcp.ini rtps_disc_group.ini/;

for my $cfg (@configs) {

  my $test = new PerlDDS::TestFramework();
  $test->enable_console_logging();
  $test->process('test', 'RtpsDiscoveryTest', "-DCPSConfigFile $cfg");

  $test->start_process('test');
  my $res = $test->finish(150);
  if ($res != 0) {
    print STDERR "ERROR: test with $cfg returned $res\n";
    $result += $res;
  }
}

exit $result if $PerlDDS::SafetyProfile;

sub run2proc {
  my $arg4proc2 = shift;
  my $description = shift;
  print "Running sedp discovery leak test ($description)\n";

  my $test = new PerlDDS::TestFramework();
  $test->enable_console_logging();
  $test->process('test1', 'RtpsDiscoveryTest',
                 '-DCPSConfigFile rtps_disc_group.ini');
  $test->process('test2', 'RtpsDiscoveryTest',
                 '-DCPSConfigFile rtps_disc_group.ini ' . $arg4proc2);
  $ENV{'OPENDDS_RTPS_DEFAULT_D0'} = $original_d0;
  $test->start_process('test1');
  $ENV{'OPENDDS_RTPS_DEFAULT_D0'} = $original_d0 + 100;
  $test->start_process('test2');
  my $res = $test->finish(150);
  if ($res != 0) {
    print STDERR "ERROR: sedp discovery leak test ($description) "
        . "returned $res\n";
  }
  return $res;
}

$result += run2proc('-value_base 100', 'different user data');
$result += run2proc('', 'same user data');

exit $result;
