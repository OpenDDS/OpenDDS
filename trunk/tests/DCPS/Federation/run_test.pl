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
my $for_monitor_test;
#
# Specific options.
#
my $interval  = 10;
my $repoCount = 2;
my $pubCount  = 1;
my $subCount  = 1;
my $samples   = 10;
my $sample_interval   = 0;
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
            "samples|n=s"   => \$samples,
            "SampleInterval|i=s"   => \$sample_interval,
            "monitor|m"       => \$for_monitor_test,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;

pod2usage( 1) and die "Not enough repositories specified!" if $repoCount < 2;

print "Debug==$debug\n" if $debug;
print "TransportDebug==$transport\n" if $transport;
print "Repos==$repoCount\n" if $verbose;
print "pubs==$pubCount\n" if $verbose;
print "subs==$subCount\n" if $verbose;
print "samples==$samples\n" if $verbose;
print "SampleInterval==$sample_interval\n" if $verbose;
print "for_monitor_test==$for_monitor_test\n" if $verbose;

my @repo_ior;
my @repo_ini;
my @repo_port;
my @repo_endpoint;
for my $index ( 1 .. $repoCount) {
  $repo_ior[ $index - 1] = PerlACE::LocalFile( "repo" . $index . ".ior");
#  $repo_ini[ $index - 1] = PerlACE::LocalFile( "repo" . $index . "-federation.ini");
  $repo_port[ $index - 1] = PerlACE::random_port();
  $repo_endpoint[ $index - 1] = "iiop://localhost:" . $repo_port[ $index - 1];
  $repo_manager[ $index - 1] = "corbaloc:iiop:localhost:" . $repo_port[ $index - 1] . "/Federator";
  print "Processing repository $index information.\n" if $debug;
}

my $publisher_ini  = PerlACE::LocalFile ("publisher.ini");
my $subscriber_ini = PerlACE::LocalFile ("subscriber.ini");

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
$transportDebug = $transport if $transport;

my $verboseDebug;
$verboseDebug = "-ORBVerboseLogging 1 " if $verbose;

my $repoOpts = "$svc_config ";
$repoOpts .= $verboseDebug if $verboseDebug;
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$repoOpts .= "-ORBLogFile $debugFile " if ($repoDebug or $transportDebug) and $debugFile;

my @repoArgs;
for my $index ( 1 .. $repoCount) {
  my $federationId = 1024*$index;
  $repoArgs[ $index - 1] .= "$repoOpts -ORBListenEndpoints " .  $repo_endpoint[ $index - 1] . " ";
  $repoArgs[ $index - 1] .= "-FederatorConfig " .  $repo_ini[ $index - 1]  . " " if $repo_ini[ $index - 1];
  $repoArgs[ $index - 1] .= "-FederationId $federationId ";
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

if ($for_monitor_test == 1) {
    $svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
      : "-ORBSvcConf ../../../tools/odds_monitor/monitor.conf ";
}

my $appOpts = "$svc_config ";

$appOpts .= $verboseDebug if $verboseDebug;
$appOpts .= "-DCPSDebugLevel $appDebug "                if $appDebug;
$appOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$appOpts .= "-ORBLogFile $debugFile " if ($appDebug or $transportDebug) and $debugFile;

  #
  # publisher --> repo2 <--> repo1 <--> repo3 <-- subscriber
  #

my @pubArgs;
for my $index ( 1 .. $subCount) {
  my $repoIndex = $index;
  $pubArgs[ $index - 1] .= "$appOpts -Samples $samples -SampleInterval $sample_interval ";
  $pubArgs[ $index - 1] .= "-DCPSInfoRepo file://$repo_ior[ $repoIndex] ";

  if (PerlACE::is_vxworks_test()) {
    $PUB[ $index - 1]  = new PerlACE::ProcessVX ("publisher", $pubArgs[ $index - 1]);
  } else {
    $PUB[ $index - 1]  = new PerlACE::Process ("publisher", $pubArgs[ $index - 1]);
  }
}

my @subArgs;
for my $index ( 1 .. $subCount) {
  my $repoIndex = (1 + $index) % $repoCount;
  $subArgs[ $index - 1] .= "$appOpts -Samples $samples ";
  $subArgs[ $index - 1] .= "-DCPSInfoRepo file://$repo_ior[ $repoIndex] ";

  if (PerlACE::is_vxworks_test()) {
    $SUB[ $index - 1]  = new PerlACE::ProcessVX ("subscriber", $subArgs[ $index - 1]);
  } else {
    $SUB[ $index - 1]  = new PerlACE::Process ("subscriber", $subArgs[ $index - 1]);
  }
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
print "\nLetting repository federation operations settle a bit...\n"; #sleep 2*$interval;

# Fire up the subscribers.

for my $index ( 1 .. $subCount) {
  print "\nSUBSCRIBER $index\n";
  print $SUB[ $index - 1]->CommandLine() . "\n";
  $SUB[ $index - 1]->Spawn();
  sleep $interval;
}
print "\nLetting subscribers settle a bit...\n"; sleep $interval;

# Fire up the publishers.

for my $index ( 1 .. $pubCount) {
  print "\nPUBLISHER $index\n";
  print $PUB[ $index - 1]->CommandLine() . "\n";
  $PUB[ $index - 1]->Spawn();
  sleep $interval;
}

# Wait for the subscribers to terminate nicely, kill them after 5 minutes
# otherwise.

my $killDelay = 300;
for my $index ( 1 .. $subCount) {
  $status = $SUB[ $index - 1]->WaitKill( $killDelay);
  if( $status != 0) {
      print STDERR "ERROR: Subscriber returned $status\n";
  }
  $failed += $status;
  $killDelay = 15;
}

# Terminate the publishers.

for my $index ( 1 .. $pubCount) {
  $status = $PUB[ $index - 1]->WaitKill( $killDelay);
  if( $status != 0) {
      print STDERR "ERROR: Publisher returned $status\n";
  }
  $failed += $status;
}

# Terminate the repositories.

for my $index ( 1 .. $repoCount) {
# for my $index ( $repoCount .. 1) {
  print "\nStopping repository $index\n";
  $status = $REPO[ $index - 1]->TerminateWaitKill(5);
  if( $status != 0) {
    print STDERR "ERROR: Repository $index returned $status\n";
  }
  $failed += $status;
}

# Clean up.

unlink @repo_ior;

# Report results.

if ($failed == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;

