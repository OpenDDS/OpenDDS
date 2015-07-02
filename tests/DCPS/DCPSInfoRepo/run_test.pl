eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;
my $debug = 1;
my $is_rtps_disc = 0;

my $dcpsrepo_ior = "dcps_ir.ior";
unlink $dcpsrepo_ior;

my $opts = "";

if ($ARGV[0] eq "rtps_disc") {
  $is_rtps_disc = 1;
  $opts .= '-r';
} else {
  $opts .= "-k file://$dcpsrepo_ior";
}

my $DCPSREPO = undef;
unless ($is_rtps_disc) {
  $DCPSREPO = PerlDDS::create_process("$DDS_ROOT/bin/DCPSInfoRepo",
                                       "-NOBITS -o $dcpsrepo_ior");
}

my $PUBSUB = PerlDDS::create_process("pubsub", "$opts -q");

unless ($is_rtps_disc) {
  print $DCPSREPO->CommandLine() . "\n" if $debug;
  $DCPSREPO->Spawn();
  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
    $DCPSREPO->Kill(); $DCPSREPO->TimedWait(1);
    exit 1;
  }
}

print $PUBSUB->CommandLine() . "\n" if $debug;
$PUBSUB->Spawn();
sleep 2;

my $PubSubResult = $PUBSUB->WaitKill(35);

if ($PubSubResult != 0) {
    print STDERR "ERROR: pubsub returned $PubSubResult\n";
    $status = 1;
}

unless ($is_rtps_disc) {
  my $ir = $DCPSREPO->TerminateWaitKill(30);

  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }
  unlink $dcpsrepo_ior;
}

exit $status;
