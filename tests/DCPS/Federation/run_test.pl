eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

PerlACE::add_lib_path('../FooType5');

use Getopt::Long qw( :config bundling) ;
use Pod::Usage ;

my $status = 0;
my $failed = 0;

#
# Basic options.
#
my $debug ;
my $man ;
my $help ;
my $verbose ;

#
# Specific options.
#
my $repoCount;
my $pubCount;
my $subCount;
my $debugFile;
my $transport;

########################################################################
#
# Process the command line.
#
GetOptions( "verbose!"      => \$verbose,
            "v"             => \$verbose,
            "help|?"        => \$help,
            "man"           => \$man,
            "debug|d=i"     => \$debug,
            "transport|t=i" => \$transport,
            "repos|r=i"     => \$repoCount,
            "pubs|p=i"      => \$pubCount,
            "subs|s=i"      => \$subCount,
            "dfile|f=s"     => \$debugFile,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;

$repoCount = 2 if not $repoCount;
pod2usage( 1) and die "Not enough repositories specified!" if $repoCount < 2;

print "Debug==$debug\n" if $debug;
print "Repos==$repoCount\n" if $debug;
print "pubs==$pubCount\n" if $debug;
print "subs==$subCount\n" if $debug;

my $samples = 10;

my @repo_ior;
my @repo_ini;
my @repo_port;
my @repo_endpoint;
for my $index ( 1 .. $repoCount) {
  $repo_ior[ $index - 1] = PerlACE::LocalFile( "repo" . $index . ".ior");
  $repo_ini[ $index - 1] = PerlACE::LocalFile( "repo" . $index . "-federation.ini");
  $repo_port[ $index - 1] = PerlACE::random_port();
  $repo_endpoint[ $index - 1] = "iiop://localhost:" . $repo_port[ $index - 1];
  $repo_manager[ $index - 1] = "corbaloc:iiop:localhost:" . $repo_port[ $index - 1] . "/Federator";
  print "Processing repository $index information.\n" if $debug;
}

my $publisher_ini  = PerlACE::LocalFile ("publisher.ini");
my $subscriber_ini = PerlACE::LocalFile ("subscriber.ini");

# Change how test is configured according to which test we are.

my $publisher_config  = "";
my $subscriber_config = "";

# Clean out any left overs from a previous run.
unlink @repo_ior;
unlink $debugFile if $debugFile;

# Configure the repositories.

my $svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf ../../tcp.conf ";


my @REPO;
my @PUB;
my @SUB;

my $PUBLISHER;
my $SUBSCRIBER;

my $repoDebug;
my $appDebug;
my $transportDebug;
$repoDebug = $debug if $debug;
$appDebug  = $debug if $debug;
$transportDebug = $transportDebug if $transport;
# $repoDebug = 10;
# $appDebug  = 10;
# $transportDebug = 10;
# $debugFile = "debug.out";

my $repoOpts = "$svc_config ";
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$repoOpts .= "-ORBLogFile $debugFile " if ($repoDebug or $transportDebug) and $debugFile;

my @repoArgs;
for my $index ( 1 .. $repoCount) {
  $repoArgs[ $index - 1] .= "$repoOpts -ORBListenEndpoints " .  $repo_endpoint[ $index - 1] . " ";
#  $repoArgs[ $index - 1] .= "-FederatorConfig " .  $repo_ini[ $index - 1] . " "
#                            if $repo_ini[ $index - 1];
  $repoArgs[ $index - 1] .= "-FederationId $index ";
  $repoArgs[ $index - 1] .= "-FederateWith " .  $repo_manager[ $index - 2] . " " if $index > 1;
  $repoArgs[ $index - 1] .= "-o " . $repo_ior[ $index - 1] . " ";

  if (PerlACE::is_vxworks_test()) {
    $REPO[ $index - 1] = new PerlACE::ProcessVX(
                               "$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repoArgs[ $index - 1]
                             );
  } else {
    $REPO[ $index - 1] = new PerlACE::Process(
                               "$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repoArgs[ $index - 1]
                             );
  }
  print "Established repository $index.\n" if $debug;
}

my $appOpts = "$svc_config ";
$appOpts .= "-DCPSDebugLevel $appDebug "                if $appDebug;
$appOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$appOpts .= "-ORBLogFile $debugFile " if ($appDebug or $transportDebug) and $debugFile;

  #
  # publisher --> repo2 <--> repo1 <--> repo3 <-- subscriber
  #

my $publisher_args  = "$svc_config -Samples $samples ";
my $subscriber_args = "$svc_config -Samples $samples ";
  $publisher_args  .= "-DCPSInfoRepo file://$repo2_ior -Samples $samples ";
  $subscriber_args .= "-DCPSInfoRepo file://$repo3_ior -Samples $samples ";

if (PerlACE::is_vxworks_test()) {
  $PUBLISHER  = new PerlACE::ProcessVX ("publisher", $publisher_args);
  $SUBSCRIBER = new PerlACE::ProcessVX ("subscriber", $subscriber_args);

} else {
  $PUBLISHER  = new PerlACE::Process ("publisher", $publisher_args);
  $SUBSCRIBER = new PerlACE::Process ("subscriber", $subscriber_args);

}

# Fire up the repositories.

for my $index ( 1 .. $repoCount) {
  print "\nREPOSITORY $index\n";
  print $REPO[ $index - 1]->CommandLine() . "\n";
  $REPO[ $index - 1]->Spawn();
  if( PerlACE::waitforfile_timed( $repo_ior[ $index - 1], 30) == -1) {
      print STDERR "ERROR: waiting for repository $index IOR file\n";
      for my $inner (1 .. $index) {
        $REPO[ $inner - 1]->Kill ();
      }
      exit 1;
  }
}

print "\n\tPROCESSING...\n"; sleep 5;

# Terminate the repositories.

for my $index ( 1 .. $repoCount) {
  $status = $REPO[ $index - 1]->TerminateWaitKill(5);
  if( $status != 0) {
    print STDERR "ERROR: Repository $index returned $status\n";
  }
  $failed += $status;
}

exit;

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

