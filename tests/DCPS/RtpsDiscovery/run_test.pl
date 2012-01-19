eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $result = 0;
my @configs = qw/dcps.ini dcps_rtps_udp.ini/;

for my $cfg (@configs) {

  my $TEST = PerlDDS::create_process('RtpsDiscoveryTest',
                                     "-DCPSConfigFile $cfg");
  print "Running with config file: $cfg\n";
  my $res = $TEST->SpawnWaitKill(150);
  if ($res != 0) {
    print STDERR "ERROR: test with $cfg returned $res\n";
    $result += $res;
  }
}

exit $result;
