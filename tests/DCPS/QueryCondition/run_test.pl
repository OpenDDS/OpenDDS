eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $opts = '';
my $dcpsrepo_ior = "repo.ior";
my $is_rtps_disc = 0;
my $DCPScfg = "dcps.ini";
my $DCPSREPO;
unlink $dcpsrepo_ior;

while (scalar @ARGV) {
  if ($ARGV[0] =~ /^-d/i) {
    shift;
    $opts .= " -DCPSTransportDebugLevel 6 -DCPSDebugLevel 10";
  }
  elsif ($ARGV[0] == 'rtps_disc') {
    $is_rtps_disc = 1;
    $DCPScfg = "rtps_disc.ini";
    shift;
  }
}

unless($is_rtps_disc) {
  $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                          "-NOBITS -o $dcpsrepo_ior");

  print STDERR $DCPSREPO->CommandLine () . "\n";
  $DCPSREPO->Spawn ();
  if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
      print STDERR "ERROR: waiting for Info Repo IOR file\n";
      $DCPSREPO->Kill ();
      exit 1;
  }
}

my $TEST = PerlDDS::create_process ('QueryConditionTest',
                                    "-DCPSConfigFile $DCPScfg -DCPSBit 0 $opts");
print STDERR $TEST->CommandLine () . "\n";
my $result = $TEST->SpawnWaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
}

unless ($is_rtps_disc) {
  $DCPSREPO->TerminateWaitKill(5);
}

exit (($result == 0) ? 0 : 1);
