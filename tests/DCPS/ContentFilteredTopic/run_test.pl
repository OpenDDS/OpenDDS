eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $dcpsrepo_ior = "repo.ior";
my $DCPSREPO;
my $DCPScfg = "dcps.ini";

unlink $dcpsrepo_ior;

my $is_rtps_disc = 0;
my $opts = '';
while (scalar @ARGV) {
  if ($ARGV[0] =~ /^-d/i) {
    shift;
    $opts .= " -DCPSTransportDebugLevel 6 -DCPSDebugLevel 10";
  }
  elsif ($ARGV[0] eq 'nopub') {
    shift;
    $opts .= " -DCPSPublisherContentFilter 0";
  }
  elsif ($ARGV[0] == 'rtps_disc') {
    $is_rtps_disc = 1;
    $DCPScfg = "rtps_disc.ini";
    shift;
  }
}

unless($is_rtps_disc) {
  $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "-NOBITS -o $dcpsrepo_ior");

  $DCPSREPO->Spawn ();
  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
      print STDERR "ERROR: waiting for Info Repo IOR file\n";
      $DCPSREPO->Kill();
      exit 1;
  }
}


my $TEST = PerlDDS::create_process('ContentFilteredTopicTest',
                                   "-DCPSConfigFile $DCPScfg ".
                                   $opts);
my $result = $TEST->SpawnWaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
}

unless ($is_rtps_disc) {
  $DCPSREPO->TerminateWaitKill(5);
}

exit (($result == 0) ? 0 : 1);
