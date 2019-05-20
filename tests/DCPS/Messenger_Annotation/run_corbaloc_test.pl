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

$repo_bit_opt = '';
$corbaloc_prefix = 'corbaloc:iiop:';
$corbaloc_suffix = '/DCPSInfoRepo';
$pub_ini = '-DCPSConfigFile pub.ini';
$sub_ini = '-DCPSConfigFile sub.ini';
  if ($ARGV[0] eq 'udp' || $ARGV[1] eq 'udp') {
    $pub_ini = '-DCPSConfigFile pub_udp.ini';
    $sub_ini = '-DCPSConfigFile sub_udp.ini';
  }

  if ($ARGV[0] eq 'host_port_only' || $ARGV[1] eq 'host_port_only') {
    $corbaloc_prefix = '';
    $corbaloc_suffix = '';
  }


my($port1) = PerlACE::random_port();
$dcpsrepo_ior = "repo.ior";
$common_args = "-DCPSInfoRepo " . "$corbaloc_prefix" . "localhost:$port1"
    . $corbaloc_suffix;

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                  "$repo_bit_opt -o $dcpsrepo_ior "
                                . "-ORBEndpoint iiop://localhost:$port1");
$Subscriber = PerlDDS::create_process ("subscriber",
                                       "$sub_ini $common_args");
$Publisher = PerlDDS::create_process ("publisher",
                                      "$pub_ini $common_args");

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}
unlink $dcpsrepo_ior;

$Publisher->Spawn ();

$Subscriber->Spawn ();

$PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
