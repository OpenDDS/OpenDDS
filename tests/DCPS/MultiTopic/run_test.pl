eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $is_rtps_disc = 0;
my $dir;
foreach my $arg (@ARGV) {
  if ($arg eq 'rtps_disc') {
    $is_rtps_disc = 1;
  }
  if ($arg eq 'cpp11' || $arg eq 'classic') {
    $dir = $arg;
  }
}

my $DCPSREPO;
my $args;
if ($is_rtps_disc) {
  $args = ' -DCPSConfigFile rtps_disc.ini';
} else {
  my $dcpsrepo_ior = 'repo.ior';
  unlink $dcpsrepo_ior;
  $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "-o $dcpsrepo_ior");
  $DCPSREPO->Spawn();
  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
  }
}

my $TEST = PerlDDS::create_process("$dir/MultiTopicTest", $args);

my $result = $TEST->SpawnWaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
}

if ($DCPSREPO) {
  $DCPSREPO->TerminateWaitKill(5);
}

exit (($result == 0) ? 0 : 1);
