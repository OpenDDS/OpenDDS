eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

my $status = 0;
my $failed = 0;

PerlACE::add_lib_path('../FooType5');

# Fire up three repositories if wanted.
# USAGE: ./run_test.pl 3
my $thirdRepo;
   $thirdRepo = 1 if $ARGV[0] == 3;

# Name the pieces.

my $samples = 10;

my $domains1_file = PerlACE::LocalFile ("domain1_ids");
my $domains2_file = PerlACE::LocalFile ("domain2_ids");
my $domains3_file = PerlACE::LocalFile ("domain3_ids");

my $repo1_ior = PerlACE::LocalFile ("repo1.ior");
my $repo2_ior = PerlACE::LocalFile ("repo2.ior");
my $repo3_ior = PerlACE::LocalFile ("repo3.ior");

my $publisher_ini  = PerlACE::LocalFile ("publisher.ini");
my $subscriber_ini = PerlACE::LocalFile ("subscriber.ini");

# Change how test is configured according to which test we are.

my $publisher_config  = "";
my $subscriber_config = "";

# Clean out any left overs from a previous run.

unlink $repo1_ior;
unlink $repo2_ior;
unlink $repo3_ior;

# Configure the repositories.

my $svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf ../../tcp.conf ";

my $repo1_endpoint = "localhost:4004";
my $repo2_endpoint = "localhost:8008";
my $repo3_endpoint = "localhost:8080";

# Declare these at file scope since we will be defining them inside
# blocks and using them outside the blocks.

my $repo1_args;
my $repo2_args;
my $repo3_args;
my $publisher_args  = "$svc_config -Samples $samples ";
my $subscriber_args = "$svc_config -Samples $samples ";
my $REPO1;
my $REPO2;
my $REPO3;
my $PUBLISHER;
my $SUBSCRIBER;

if( $thirdRepo) {
  #
  # publisher --> repo2 <--> repo1 <--> repo3 <-- subscriber
  #

  $repo1_args = "$svc_config -o $repo1_ior -d $domains1_file "
#              . "-ORBDebugLevel 1 "
              . "-f 1957 ";

  $repo2_args = "$svc_config -o $repo2_ior -d $domains2_file "
#              . "-ORBDebugLevel 1 "
              . "-f 1975 -j $repo1_endpoint ";

  $repo3_args = "$svc_config -o $repo3_ior -d $domains3_file "
#              . "-ORBDebugLevel 1 "
              . "-f 1982 -j $repo1_endpoint ";

  $publisher_args  .= "-DCPSInfoRepo file://$repo2_ior -Samples $samples ";
  $subscriber_args .= "-DCPSInfoRepo file://$repo3_ior -Samples $samples ";

} else {
  #
  # publisher --> repo1 <--> repo2 <-- subscriber
  #

  $repo1_args = "$svc_config -o $repo1_ior -d $domains1_file "
#              . "-ORBDebugLevel 1 "
              . "-f 1957 ";

  $repo2_args = "$svc_config -o $repo2_ior -d $domains2_file "
#              . "-ORBDebugLevel 1 "
              . "-f 1975 -j $repo1_endpoint ";

  $publisher_args  .= "-DCPSInfoRepo file://$repo1_ior -Samples $samples ";
  $subscriber_args .= "-DCPSInfoRepo file://$repo2_ior -Samples $samples ";
}

if (PerlACE::is_vxworks_test()) {
  $REPO1 = new PerlACE::ProcessVX ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo1_args);
  $REPO2 = new PerlACE::ProcessVX ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo2_args);
  $REPO3 = new PerlACE::ProcessVX ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo3_args) if( $thirdRepo);

  $PUBLISHER  = new PerlACE::ProcessVX ("publisher", $publisher_args);
  $SUBSCRIBER = new PerlACE::ProcessVX ("subscriber", $subscriber_args);

} else {
  $REPO1 = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo1_args);
  $REPO2 = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo2_args);
  $REPO3 = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo3_args) if( $thirdRepo);

  $PUBLISHER  = new PerlACE::Process ("publisher", $publisher_args);
  $SUBSCRIBER = new PerlACE::Process ("subscriber", $subscriber_args);

}

# Fire up the repositories.

print $REPO1->CommandLine(), "\n";
$REPO1->Spawn ();
if (PerlACE::waitforfile_timed ($repo1_ior, 30) == -1) {
    print STDERR "ERROR: waiting for repository 1 IOR file\n";
    $REPO1->Kill ();
    exit 1;
}

print $REPO2->CommandLine(), "\n";
$REPO2->Spawn ();
if (PerlACE::waitforfile_timed ($repo2_ior, 30) == -1) {
    print STDERR "ERROR: waiting for repository 2 IOR file\n";
    $REPO2->Kill ();
    $REPO1->Kill ();
    exit 1;
}

if( $thirdRepo) {
  print $REPO3->CommandLine(), "\n";
  $REPO3->Spawn ();
  if (PerlACE::waitforfile_timed ($repo3_ior, 30) == -1) {
      print STDERR "ERROR: waiting for repository 3 IOR file\n";
      $REPO3->Kill ();
      $REPO2->Kill ();
      $REPO1->Kill ();
      exit 1;
  }
}

# Fire up the publisher
print $PUBLISHER->CommandLine(), "\n";
$PUBLISHER->Spawn ();

# Fire up the subscriber
print $SUBSCRIBER->CommandLine(), "\n";
$SUBSCRIBER->Spawn ();

# Wait up to 5 minutes for test to complete.

$status = $SUBSCRIBER->WaitKill (300);
if ($status != 0) {
    print STDERR "ERROR: Subscriber returned $status\n";
}
$failed += $status;

# And it can, in the worst case, take up to half a minute to shut down the rest.

$status = $PUBLISHER->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: Publisher returned $status\n";
}
$failed += $status;

$status = $REPO1->TerminateWaitKill(5);
if ($status != 0) {
    print STDERR "ERROR: Repository 1 returned $status\n";
}
$failed += $status;

$status = $REPO2->TerminateWaitKill(5);
if ($status != 0) {
    print STDERR "ERROR: Repository 2 returned $status\n";
}
$failed += $status;

if( $thirdRepo) {
  $status = $REPO3->TerminateWaitKill(5);
  if ($status != 0) {
      print STDERR "ERROR: Repository 3 returned $status\n";
  }
  $failed += $status;
}

# Clean up.

unlink $repo1_ior;
unlink $repo2_ior;
unlink $repo3_ior;

# Report results.

if ($failed == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;

