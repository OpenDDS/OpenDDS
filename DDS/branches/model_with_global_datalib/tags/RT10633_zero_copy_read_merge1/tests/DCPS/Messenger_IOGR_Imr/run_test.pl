eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;
use Sys::Hostname;

$useImr = 1;
if ($ARGV[0] eq 'noImr') {
    $useImr = 0;
}

$status = 0;

#my $OBJ_REF_STYLE = "-orbobjrefstyle url";
my $domains_file = PerlACE::LocalFile ("domain_ids");
my $dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
my $dcpsrepo_ior2 = PerlACE::LocalFile ("repo2.ior");
my $dcpsrepo_iogr = PerlACE::LocalFile ("repo.iogr");
my $driver_trigger = PerlACE::LocalFile ("driver_trigger");
my $publisher_trigger = PerlACE::LocalFile ("publisher_trigger");
my $implrepo_ior = PerlACE::LocalFile ("imr.ior");
my $activator_ior = PerlACE::LocalFile ("activator.ior");
my $imr_init_ref = "-ORBInitRef ImplRepoService=file://$implrepo_ior";
my $implrepo_server = "$ENV{TAO_ROOT}/orbsvcs/ImplRepo_Service/ImplRepo_Service";
my $imr_activator = "$ENV{TAO_ROOT}/orbsvcs/ImplRepo_Service/ImR_Activator";
my $tao_imr = "$ENV{ACE_ROOT}/bin/tao_imr";
my $RepoOpts = "-NOBITS -o $dcpsrepo_ior -d $domains_file $OBJ_REF_STYLE "
    . "-ORBEndPoint iiop://:12345";
my $repo_port = "12346";
my $RepoOpts2 = "-NOBITS -o $dcpsrepo_ior2 -d $domains_file $OBJ_REF_STYLE "
    . "-ORBEndPoint iiop://:$repo_port";
if ($useImr == 1) {
    $RepoOpts = $RepoOpts . " -ORBuseimr 1 $imr_init_ref";
}
my $AGGREGATOR = new PerlACE::Process ("Aggregator", "-a file://$dcpsrepo_ior "
				    . "-b file://$dcpsrepo_ior2 -c $dcpsrepo_iogr");

my $ImR = new PerlACE::Process ($implrepo_server, "-o $implrepo_ior $OBJ_REF_STYLE "
				. "-orbendpoint iiop://:12347");
my $Act = new PerlACE::Process ($imr_activator, "-o $activator_ior $imr_init_ref "
				. "$OBJ_REF_STYLE -orbendpoint iiop://:12348");
my $DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $RepoOpts);
my $DCPSREPO2 = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $RepoOpts2);
my $imr_util = new PerlACE::Process ("$tao_imr");

my $opts =  "-ORBSvcConf ../../tcp.conf";
my $pub_opts = "$opts -DCPSConfigFile pub.ini -orbendpoint iiop://:12349";
my $sub_opts = "$opts -DCPSConfigFile sub.ini -orbendpoint iiop://:12350";

$Subscriber = new PerlACE::Process ("subscriber", " $sub_opts");
$Publisher = new PerlACE::Process ("publisher", " $pub_opts -orbdebuglevel 10");

# We want the tao_imr executable to be found exactly in the path
# given, without being modified by the value of -ExeSubDir.
# So, we tell its Process object to ignore the setting of -ExeSubDir.
$imr_util->IgnoreExeSubDir(1);

sub CleanupOutput {
    unlink $dcpsrepo_ior;
    unlink $dcpsrepo_ior2;
    unlink $dcpsrepo_iogr;
    unlink $implrepo_ior;
    unlink $activator_ior;
    unlink $driver_trigger;
    unlink $publisher_trigger;
}

sub RunImRUtil {
    my $cmd = shift;
    $imr_util->Arguments("$imr_init_ref $cmd");
    print ">>> " . $imr_util->CommandLine() . "\n";
    return $imr_util->SpawnWaitKill(5);
}

sub WaitForFile {
    my $file = shift;
    my $timeout = shift;

    my $ret = PerlACE::waitforfile_timed($file, $timeout);
    if ($ret == -1) {
        print STDERR "ERROR: Cannot find file <$file>\n";
    }
    return $ret;
}

sub SpawnWait {
    my $process = shift;
    my $file = shift;
    my $timeout = shift;

    print ">>> " . $process->CommandLine() . "\n";
    $process->Spawn();
    return WaitForFile ($file, $timeout);
}

sub HardKillAll {
    $ImR->Kill();
    $Act->Kill();
    $DCPSREPO->Kill ();
    $DCPSREPO2->Kill ();
}

CleanupOutput();

if ($useImr == 1) {
    if (SpawnWait($ImR, $implrepo_ior, 10) != 0) {
	$ImR->Kill();
	exit 1;
    }

    if (SpawnWait($Act, $activator_ior, 10) != 0) {
	$ImR->Kill();
	$Act->Kill();
	exit 1;
    }
}

#instead of using tao_imr add, we'll use tao_imr update, because
#we want to run the server once to generate the ior file.

if (SpawnWait($DCPSREPO, $dcpsrepo_ior, 30) != 0) {
    HardKillAll ();
    exit 1;
}

if (SpawnWait($DCPSREPO2, $dcpsrepo_ior2, 30) != 0) {
    HardKillAll ();
    exit 1;
}

open(OUTP, ">$dcpsrepo_ior2") or die("Cannot open file '$dcpsrepo_ior2' for writing\n");
print OUTP "corbaloc:::$repo_port/InfoRepo";
close OUTP;

# Generate IOGR file.
if (SpawnWait($AGGREGATOR, $dcpsrepo_iogr, 30) != 0) {
    HardKillAll ();
    exit 1;
}

if ($useImr == 1)
{
# Note : If the server registers itself, then it won't set the
# activator name. If we don't set it here, then the activator
# won't be able to start the server.
    my $actname = hostname;
    my $repoExe = $DCPSREPO->Executable();
    RunImRUtil("update InfoRepo -l $actname -c \"$repoExe $RepoOpts\"");
    RunImRUtil("list -v");
}

# The publisher invocation should incarnate the InfoRepo.
print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

# wait for driver_trigger.
WaitForFile ($driver_trigger, 30);
unlink $driver_trigger;
$Act->Kill();
$DCPSREPO->Kill();
# create repo_ready_trigger_file.
open(OUTP, ">$publisher_trigger") or die("Cannot open file '$publisher_trigger' for writing\n");
print OUTP "trigger\n";
close OUTP;
#generate_test_file ($repo_ready_trigger_file, 0);

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

$PublisherResult = $Publisher->WaitKill (30);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

if ($useImr == 1) {
    RunImRUtil("shutdown InfoRepo");
}
HardKillAll ();

#CleanupOutput();

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
