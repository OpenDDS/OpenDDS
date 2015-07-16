eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

$dcpsrepo_ior = "dcps_ir.ior";

unlink $dcpsrepo_ior;
my $is_rtps_disc = 0;
my $options = '';
my $DCPSREPO;

PerlDDS::add_lib_path('../FooType');

if ($ARGV[0] eq 'rtps_disc') {
  $options = "-DCPSConfigFile " . $ARGV[0] . ".ini";
  $is_rtps_disc = 1;
}
elsif ($ARGV[0] eq 'rtps_disc_tcp') {
  $options = "-DCPSConfigFile " . $ARGV[0] . ".ini";
  $is_rtps_disc = 1;
}

if (!$is_rtps_disc) {
  $options .= "-DCPSInfoRepo file://$dcpsrepo_ior";
  $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior ");
}

$Topic = PerlDDS::create_process ("topic_test", "$options");

if (!$is_rtps_disc) {
  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn ();
  if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
      print STDERR "ERROR: cannot find file <$dcpsrepo_ior>\n";
      $DCPSREPO->Kill (); $DCPSREPO->TimedWait (1);
      exit 1;
  }
}

print $Topic->CommandLine() . "\n";

$TopicResult = $Topic->SpawnWaitKill (60);

if ($TopicResult != 0) {
    print STDERR "ERROR: topic_test returned $TopicResult\n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(30) if (!$is_rtps_disc);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

exit $status;
