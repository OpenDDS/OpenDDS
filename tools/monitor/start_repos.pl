eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (ACE_ROOT);
use Env (DDS_ROOT);
use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use PerlDDS::Run_Test;

use Getopt::Long qw( :config bundling) ;
use Pod::Usage ;

my $status = 0;

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
my $repoCount = 2;
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
            "dfile|f=s"     => \$debugFile,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;

print "Debug==$debug\n" if $debug and $verbose;
print "TransportDebug==$transport\n" if $transport and $verbose;
print "Repos==$repoCount\n" if $verbose;

my @repo_ior;
my @repo_ini;
my @repo_port;
my @repo_endpoint;
for my $index ( 1 .. $repoCount) {
  $repo_ior[ $index - 1] = PerlACE::LocalFile( "repo" . $index . ".ior");
#  $repo_ini[ $index - 1] = PerlACE::LocalFile( "repo" . $index . "-federation.ini");
  $repo_port[ $index - 1] = PerlACE::random_port();
  $repo_endpoint[ $index - 1] = "iiop://localhost:" . $repo_port[ $index - 1];
  print "Processing repository $index information.\n" if $debug;
}

# Clean out any left overs from a previous run.
unlink @repo_ior;
unlink $debugFile if $debugFile;

# Configure the repositories.



my @REPO;

my $repoDebug;
my $transportDebug;
$repoDebug = $debug if $debug;
$transportDebug = $transport if $transport;

my $verboseDebug;
$verboseDebug = "-ORBVerboseLogging 1 " if $verbose;

my $repoOpts = "";
$repoOpts .= $verboseDebug if $verboseDebug;
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$repoOpts .= "-ORBLogFile $debugFile " if ($repoDebug or $transportDebug) and $debugFile;

my @repoArgs;
for my $index ( 1 .. $repoCount) {
  $repoArgs[ $index - 1] .= "$repoOpts -ORBListenEndpoints " .  $repo_endpoint[ $index - 1] . " ";
  $repoArgs[ $index - 1] .= "-o " . $repo_ior[ $index - 1] . " ";

  $REPO[ $index - 1] = PerlDDS::create_process(
                             "$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repoArgs[ $index - 1]
                       );
  print "Established repository $index.\n" if $debug;
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

print STDERR "Terminate the repositories[y]? ";
my $discard = <STDIN>;

# Terminate the repositories.

for my $index ( 1 .. $repoCount) {
  print "\nStopping repository $index\n";
  $status = $REPO[ $index - 1]->TerminateWaitKill(5);
  if( $status != 0) {
    print STDERR "ERROR: Repository $index returned $status\n";
  }
}

# Clean up.

unlink @repo_ior;

exit $status;

