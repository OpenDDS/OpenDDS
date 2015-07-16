eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use FileHandle;
use Cwd;
use File::Copy;
use strict;

my $status = 0;

my $dcpsrepo_ior = "repo.ior";
my $dcpsrepo2_ior = "repo2.ior";
my $dcpsrepo3_ior = "repo3.ior";


unlink $dcpsrepo_ior;
my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       " -o $dcpsrepo_ior");
print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

copy($dcpsrepo_ior, $dcpsrepo2_ior);
copy($dcpsrepo_ior, $dcpsrepo3_ior);

my $cfg = new PerlACE::ConfigList->check_config('NO_BUILT_IN_TOPICS')
          ? 'test1_nobits.ini' : 'test1.ini';
my $TST = PerlDDS::create_process("ConfigFile", "-DCPSConfigFile $cfg");
print $TST->CommandLine() . "\n";
my $retcode = $TST->SpawnWaitKill(60);
if ($retcode != 0) {
    $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;
unlink $dcpsrepo2_ior;
unlink $dcpsrepo3_ior;
if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
