eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $is_rtps_disc = 0;
if ($ARGV[0] eq 'rtps_disc') {
  $is_rtps_disc = 1;
}

my $DCPSREPO;
if (!$is_rtps_disc) {
  my $dcpsrepo_ior = 'repo.ior';
  unlink $dcpsrepo_ior;
  $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "-NOBITS -o $dcpsrepo_ior");
  $DCPSREPO->Spawn();
  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
  }
}

my $TEST = PerlDDS::create_process('MultiTopicTest',
                                   $is_rtps_disc
                                   ? '-DCPSConfigFile rtps_disc.ini'
                                   : '-DCPSBit 0');

my $result = $TEST->SpawnWaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
}

if ($DCPSREPO) {
  $DCPSREPO->TerminateWaitKill(5);
}

exit (($result == 0) ? 0 : 1);
