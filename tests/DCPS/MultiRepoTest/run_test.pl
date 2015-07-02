eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

my $status = 0;
my $failed = 0;
my $debug;
# $debug = 10;

PerlDDS::add_lib_path('../FooType5');

# Name the pieces.

my $samples = 10;
my $sample_interval = 0;
my $sys1_addr = "localhost:16701";
my $sys1_pub_domain = 511;
my $sys1_sub_domain = 411;
my $sys1_pub_topic  = "Sys1";
my $sys1_sub_topic  = "Sys1";
my $sys2_addr = "localhost:16703";
my $sys2_pub_domain = 811;
my $sys2_sub_domain = 711;
my $sys2_pub_topic  = "Sys2";
my $sys2_sub_topic  = "Sys2";
my $sys3_addr = "localhost:16705";
my $sys3_pub_domain = 911;
my $sys3_sub_domain = 911;
my $sys3_pub_topic  = "Left";
my $sys3_sub_topic  = "Right";
my $monitor_addr = "localhost:29803";

my $dcpsrepo1_ior = "repo1.ior";
my $dcpsrepo2_ior = "repo2.ior";
my $dcpsrepo3_ior = "repo3.ior";

my $system1_ini   = "system1.ini";
my $system2_ini   = "system2.ini";
my $system3_ini   = "system3.ini";
my $monitor_ini   = "monitor.ini";

# Change how test is configured according to which test we are.

my $system1_config = "";
my $system2_config = "";
my $system3_config = "";
my $monitor_config = "";

if ($ARGV[0] eq 'fileconfig') {
  # File configuration test.
  $system1_config = "-DCPSConfigFile $system1_ini ";
  $system2_config = "-DCPSConfigFile $system2_ini ";
  $system3_config = "-DCPSConfigFile $system3_ini ";
  $monitor_config = "-DCPSConfigFile $monitor_ini "
                  . "-WriterDomain $sys1_sub_domain -ReaderDomain $sys1_pub_domain "
                  . "-WriterDomain $sys2_sub_domain -ReaderDomain $sys2_pub_domain "
                  . "-WriterDomain $sys3_sub_domain -ReaderDomain $sys3_pub_domain ";
  print STDERR "NOTICE: file configuration test not implemented\n";

} elsif ($ARGV[0] eq '') {
  # Default: Command line configuration test.
  $system1_config = "-InfoRepo file://$dcpsrepo1_ior ";
  $system2_config = "-InfoRepo file://$dcpsrepo2_ior ";
  $system3_config = "-InfoRepo file://$dcpsrepo3_ior ";

  # This is horribly position dependent.  The InfoRepo IOR values start
  # with the first RepositoryKey value 0 and increment as they are
  # encountered with the current (in the command line) key value used to
  # bind any domain definitions from the command line.
  $monitor_config = "-InfoRepo file://$dcpsrepo1_ior "
                  . "-WriterDomain $sys1_sub_domain -ReaderDomain $sys1_pub_domain "
                  . "-InfoRepo file://$dcpsrepo2_ior "
                  . "-WriterDomain $sys2_sub_domain -ReaderDomain $sys2_pub_domain "
                  . "-InfoRepo file://$dcpsrepo3_ior "
                  . "-WriterDomain $sys3_sub_domain -ReaderDomain $sys3_pub_domain ";

} elsif ($ARGV[0] eq 'monitor') {
  # Default: Command line configuration test.
  $system1_config = "-InfoRepo file://$dcpsrepo1_ior ";
  $system2_config = "-InfoRepo file://$dcpsrepo2_ior ";
  $system3_config = "-InfoRepo file://$dcpsrepo3_ior ";

  # This is horribly position dependent.  The InfoRepo IOR values start
  # with the first RepositoryKey value 0 and increment as they are
  # encountered with the current (in the command line) key value used to
  # bind any domain definitions from the command line.
  $monitor_config = "-InfoRepo file://$dcpsrepo1_ior "
                  . "-WriterDomain $sys1_sub_domain -ReaderDomain $sys1_pub_domain "
                  . "-InfoRepo file://$dcpsrepo2_ior "
                  . "-WriterDomain $sys2_sub_domain -ReaderDomain $sys2_pub_domain "
                  . "-InfoRepo file://$dcpsrepo3_ior "
                  . "-WriterDomain $sys3_sub_domain -ReaderDomain $sys3_pub_domain ";

  $samples = 10000;
  $sample_interval = 5;
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0]\n";
  exit 1;
}

# Clean out any left overs from a previous run.

unlink $dcpsrepo1_ior;
unlink $dcpsrepo2_ior;
unlink $dcpsrepo3_ior;

# Configure the repositories.
my $opt;
$opt = "-DCPSDebugLevel $debug " if $debug;

# Configure the subsystems.
my $sys1_parameters = "$opt $system1_config "
                    .             "-WriterDomain $sys1_pub_domain -ReaderDomain $sys1_sub_domain "
                    .             "-WriterTopic  $sys1_pub_topic  -ReaderTopic  $sys1_sub_topic "
                    .             "-Address $sys1_addr ";

my $sys2_parameters = "$opt $system2_config "
                    .             "-WriterDomain $sys2_pub_domain -ReaderDomain $sys2_sub_domain "
                    .             "-WriterTopic  $sys2_pub_topic  -ReaderTopic  $sys2_sub_topic "
                    .             "-Address $sys2_addr ";

my $sys3_parameters = "$opt $system3_config "
                    .             "-WriterDomain $sys3_pub_domain -ReaderDomain $sys3_sub_domain "
                    .             "-WriterTopic  $sys3_pub_topic  -ReaderTopic  $sys3_sub_topic "
                    .             "-Address $sys3_addr ";

# Configure the monitor.

# The monitor has 3 Writers and 3 Readers, which are to be connected with
# the subsystems defined above.  The topology goes like this:
#
#   monitor writer [0]    --->   system1 reader
#   monitor reader [0]   <---    system1 writer
#
#   monitor writer [1]    --->   system2 reader
#   monitor reader [1]   <---    system2 writer
#
#   monitor writer [2]    --->   system3 reader
#   monitor reader [2]   <---    system3 writer
#

$monitor_parameters = "$opt -Samples $samples -SampleInterval $sample_interval $monitor_config "
                    . "-WriterTopic $sys1_sub_topic "
                    . "-WriterTopic $sys2_sub_topic "
                    . "-WriterTopic $sys3_sub_topic "
                    . "-ReaderTopic $sys1_pub_topic "
                    . "-ReaderTopic $sys2_pub_topic "
                    . "-ReaderTopic $sys3_pub_topic "
                    . "-Address $monitor_addr ";


$DCPSREPO1 = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                     "$opt -o $dcpsrepo1_ior "
  #                                  . " -ORBDebugLevel 1 "
                                     . "-FederationId 273 ");

$DCPSREPO2 = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                     "$opt -o $dcpsrepo2_ior "
  #                                  . " -ORBDebugLevel 1 "
                                     . "-FederationId 546 ");

$DCPSREPO3 = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                     "$opt -o $dcpsrepo3_ior "
  #                                  . " -ORBDebugLevel 1 "
                                     . "-FederationId 819 ");

$System1 = PerlDDS::create_process ("system", $sys1_parameters);

$System2 = PerlDDS::create_process ("system", $sys2_parameters);

$System3 = PerlDDS::create_process ("system", $sys3_parameters);

$Monitor = PerlDDS::create_process ("monitor", $monitor_parameters);

# Fire up the repositories.

print $DCPSREPO1->CommandLine(), "\n";
$DCPSREPO1->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo1_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo 1 IOR file\n";
    $DCPSREPO1->Kill ();
    exit 1;
}

print $DCPSREPO2->CommandLine(), "\n";
$DCPSREPO2->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo2_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo 2 IOR file\n";
    $DCPSREPO2->Kill ();
    $DCPSREPO1->Kill ();
    exit 1;
}

print $DCPSREPO3->CommandLine(), "\n";
$DCPSREPO3->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo3_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo 3 IOR file\n";
    $DCPSREPO3->Kill ();
    $DCPSREPO2->Kill ();
    $DCPSREPO1->Kill ();
    exit 1;
}

# Fire up the monitor process.

print $Monitor->CommandLine(), "\n";
$Monitor->Spawn ();

# Fire up the subsystems.

print $System1->CommandLine(), "\n";
$System1->Spawn ();

print $System2->CommandLine(), "\n";
$System2->Spawn ();

print $System3->CommandLine(), "\n";
$System3->Spawn ();

# Wait up to 5 minutes for test to complete.

$status = $Monitor->WaitKill (300);
if ($status != 0) {
    print STDERR "ERROR: monitor returned $status\n";
}
$failed += $status;

# And it can, in the worst case, take up to a minute to shut down the rest.

$status = $System1->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: system 1 returned $status\n";
}
$failed += $status;

$status = $System2->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: system 2 returned $status\n";
}
$failed += $status;

$status = $System3->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: system 3 returned $status\n";
}
$failed += $status;

$status = $DCPSREPO1->TerminateWaitKill(5);
if ($status != 0) {
    print STDERR "ERROR: DCPSInfoRepo 1 returned $status\n";
}
$failed += $status;

$status = $DCPSREPO2->TerminateWaitKill(5);
if ($status != 0) {
    print STDERR "ERROR: DCPSInfoRepo 2 returned $status\n";
}
$failed += $status;

$status = $DCPSREPO3->TerminateWaitKill(5);
if ($status != 0) {
    print STDERR "ERROR: DCPSInfoRepo 3 returned $status\n";
}
$failed += $status;

# Clean up.

unlink $dcpsrepo1_ior;
unlink $dcpsrepo2_ior;
unlink $dcpsrepo3_ior;

# Report results.

if ($failed == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $failed;

