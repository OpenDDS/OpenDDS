eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $result = 0;
my @configs = qw/rtps_disc.ini rtps_disc_tcp.ini rtps_disc_group.ini/;

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

{
    print "Running sedp discovery leak test (different user data)\n";
    my $TEST1 = PerlDDS::create_process('RtpsDiscoveryTest',
                                        "-DCPSConfigFile rtps_disc_group.ini");
    $TEST1->Spawn();
    my $TEST2 = PerlDDS::create_process('RtpsDiscoveryTest',
                                        "-DCPSConfigFile rtps_disc_group2.ini -value_base 100");
    $TEST2->Spawn();
    my $res1 = $TEST1->WaitKill(150);
    my $res2 = $TEST2->WaitKill(150);
    if ($res1 != 0 || $res2 != 0) {
        print STDERR "ERROR: sedp discovery leak test (different user data) returned $res1 $res2\n";
        $result += $res1 + $res2;
    }
}

{
    print "Running sedp discovery leak test (same user data)\n";
    my $TEST1 = PerlDDS::create_process('RtpsDiscoveryTest',
                                        "-DCPSConfigFile rtps_disc_group.ini");
    $TEST1->Spawn();
    my $TEST2 = PerlDDS::create_process('RtpsDiscoveryTest',
                                        "-DCPSConfigFile rtps_disc_group2.ini");
    $TEST2->Spawn();
    my $res1 = $TEST1->WaitKill(150);
    my $res2 = $TEST2->WaitKill(150);
    if ($res1 != 0 || $res2 != 0) {
        print STDERR "ERROR: sedp discovery leak test (same user data) returned $res1 $res2\n";
        $result += $res1 + $res2;
    }
}

exit $result;
