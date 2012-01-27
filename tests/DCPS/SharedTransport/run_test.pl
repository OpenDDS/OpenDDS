eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');
PerlDDS::add_lib_path('../TestFramework');

my $arg_idx = 0;
my $transport = "tcp.ini";
my $is_rtps_disc = 0;

if ($ARGV[$arg_idx] eq 'udp') {
  $transport = "udp.ini";
  $arg_idx = $arg_idx + 1;
}

if ($ARGV[$arg_idx] eq 'multicast') {
  $transport = "multicast.ini";
  $arg_idx = $arg_idx + 1;
}

if ($ARGV[$arg_idx] eq 'rtps') {
  $transport = "rtps.ini";
  $arg_idx = $arg_idx + 1;
}

if ($ARGV[$arg_idx] eq 'rtps_disc') {
  $transport = "rtps_disc.ini";
  $arg_idx = $arg_idx + 1;
  $is_rtps_disc = 1;
}

$test_opts = "-DCPSConfigFile $transport @ARGV";

$status = 0;

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "-o $dcpsrepo_ior ");

$Test = PerlDDS::create_process("test", "$test_opts");

unless ($is_rtps_disc) {
  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn();
  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
  }
}

print $Test->CommandLine() . "\n";
$Test->Spawn();

$TestResult = $Test->WaitKill(300);
if ($TestResult != 0) {
  print STDERR "ERROR: test returned $TestResult \n";
  $status = 1;
}

unless ($is_rtps_disc) {
  $ir = $DCPSREPO->TerminateWaitKill(5);
  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }
  unlink $dcpsrepo_ior;
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
