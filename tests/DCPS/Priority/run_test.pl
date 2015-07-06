eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env qw( DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use English;
use PerlDDS::Run_Test;

use Getopt::Long qw( :config bundling) ;
use Pod::Usage ;

my $status = 0;
my $failed = 0;

#
# Basic options.
#
my $debug;
my $man;
my $help;
my $verbose;
my $orbVerbose;
my $dFile;
my $transportDebug;
my $repoDebug;
my $noaction;
my $multipleInstances;

#
# Specific options.
#
my $transportType = "tcp";
my $priority      = "1";

########################################################################
#
# Process the command line.
#
GetOptions( "verbose!"            => \$verbose,
            "v"                   => \$verbose,
            "instances|i"         => \$multipleInstances,
            "ORBVerboseLogging|V" => \$orbVerbose,
            "help|?"              => \$help,
            "man"                 => \$man,
            "debug|d=i"           => \$debug,
            "tdebug|T=i"          => \$transportDebug,
            "rdebug|R=i"          => \$repoDebug,
            "noaction|x"          => \$noaction,
            "dfile|f=s"           => \$dFile,
            "transport|t=s"       => \$transportType,
            "priority|p=s"        => \$priority,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;
#
########################################################################

print "Priority==$priority\n" if $verbose;

$orbVerbose = 1;
$verbose = 1;
$debug = 5;
$repoDebug = 5;

# Verbosity.
print "Debug==$debug\n"                         if $verbose and $debug;
print "RepoDebug==$repoDebug\n"                 if $verbose and $repoDebug;
print "TransportDebug==$transportDebug\n"       if $verbose and $transportDebug;
print "DebugFile==$dFile\n"                     if $verbose and $dFile;
print "VerboseLogging==$orbVerbose\n"           if $verbose and $orbVerbose;
print "MultipleInstances==$multipleInstances\n" if $verbose and $multipleInstances;

# Files.
my $repo_ior  = PerlACE::LocalFile("repo.ior");
my $debugFile;
   $debugFile = PerlACE::LocalFile( $dFile) if $dFile;
my $confFile  = PerlACE::LocalFile( "services.conf");

my $iniFile;
if ("solaris" eq $OSNAME) {
  $iniFile   = PerlACE::LocalFile( "transport_$OSNAME.ini");
} else {
  $iniFile   = PerlACE::LocalFile( "transport.ini");
}

# Clean out leftovers.
unlink $repo_ior;
unlink $debugFile if $debugFile;


my $common_opts = "-DCPSConfigFile $iniFile ";
$common_opts .= "-v " if $verbose;

# Perocess variables.
my $REPO;
my $SUB;
my $PUB;

# Establish process arguments.

my $appDebug;
$appDebug  = $debug if $debug;

my $verboseDebug;
$verboseDebug = "-ORBVerboseLogging 1 " if $orbVerbose;

my $repoOpts = "";
$repoOpts .= $verboseDebug if $verboseDebug;
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$repoOpts .= "-ORBLogFile $debugFile " if ($repoDebug or $transportDebug) and $debugFile;

my $appOpts = "$common_opts ";
$appOpts .= $verboseDebug if $verboseDebug;
$appOpts .= "-DCPSDebugLevel $appDebug " if $appDebug;
$appOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$appOpts .= "-ORBLogFile $debugFile " if ($appDebug or $transportDebug) and $debugFile;

# Define the processes.

my $repoArgs = "$repoOpts -o $repo_ior ";
$REPO = PerlDDS::create_process (
            "$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repoArgs
          );

my $subArgs = "$appOpts ";
$subArgs .= "-DCPSInfoRepo file://$repo_ior ";
$subArgs .= "-DCPSGlobalTransportConfig $transportType ";
$subArgs .= "-t $transportType ";
$SUB = PerlDDS::create_process ( "subscriber", $subArgs);

my $pubArgs = "$appOpts ";
$pubArgs .= "-DCPSInfoRepo file://$repo_ior ";
$pubArgs .= "-DCPSGlobalTransportConfig $transportType ";
$pubArgs .= "-p " . $priority . " ";
$pubArgs .= "-m " if $multipleInstances;
$pubArgs .= "-t $transportType ";
$PUB = PerlDDS::create_process ( "publisher", $pubArgs);

# Be verbose.

if( $noaction) {
  print $REPO->CommandLine() . "\n" if $verbose;
  print $SUB->CommandLine() . "\n" if $verbose;
  print $PUB->CommandLine() . "\n" if $verbose;
  exit;
}

# Fire up the repository.

print "\nREPOSITORY\n";
print $REPO->CommandLine() . "\n";
$REPO->Spawn;
if( PerlACE::waitforfile_timed( $repo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for repository IOR file $repo_ior.\n";
  exit 1;
}

# Fire up the subscriber.

print "\nSUBSCRIBER\n";
print $SUB->CommandLine() . "\n";
$SUB->Spawn();

# Fire up the publisher.

print "\nPUBLISHER\n";
print $PUB->CommandLine() . "\n";
$PUB->Spawn();

# Wait for subscriber to terminate nicely.  Kill it after 5 minutes
# otherwise.

my $killDelay = 300;
$status = $SUB->WaitKill( $killDelay);
if( $status != 0) {
  print STDERR "ERROR: Subscriber returned $status\n";
  ++$failed;
}
$killDelay = 15;

# Terminate the publisher.

$status = $PUB->WaitKill( $killDelay);
if( $status != 0) {
  print STDERR "ERROR: Publisher returned $status\n";
  ++$failed;
}

# Terminate the repository.

print "\nStopping repository\n";
$status = $REPO->TerminateWaitKill( 5);
if( $status != 0) {
  print STDERR "ERROR: Repository returned $status\n";
  ++$failed;
}

# Clean up.

unlink $repo_ior;

# Report results.

if( $failed == 0) {
  print "test PASSED.\n";

} else {
  print STDERR "test FAILED.\n";
}

exit $failed;

=head1 NAME

run_test.pl - Execute a TRANSPORT_PRIORITY test.

=head1 SYNOPSIS

./run_test.pl [options]

Options:

  -? | --help            brief help message

  --man                  full documentation

  -x | --noaction        do not execute any processing

  -v | --verbose         be chatty while executing

  -V | --ORBVerboseLogging=NUMBER
                         set the corresponding ORB option

  -d NUMBER | --debug=NUMBER
                         set the DCPS debugging level

  -T NUMBER | --tdebug=NUMBER
                         set the DCPSTransportDebug debugging level

  -R NUMBER | --rdebug=NUMBER
                         set the DCPS debugging level for the repository

  -f FILE | --dfile=FILE set the filename for debug output

  -t NAME | --transport=NAME
                         use NAME transport for test execution - one of
                         (tcp, udp, multicast), default tcp

  -p NUMBER | --priority=NUMBER
                         comma separated list of priority to publish
                         during the test - default is 1

=head1 OPTIONS

=over 8

=item B<-?> | B<--help>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<-x> | B<--noaction>

Print the commands that would be executed with the current set of command
line options and exit without performing any processing.

=item B<-v> | B<--verbose>

Print additional information while executing.

=item B<-V> | B<--ORBVerboseLogging=NUMBER>

Sets the -ORBVerboseLogging option to NUMBER.

The default value is 0.

The value is set to 1 if the single letter form isused (-V).

=item B<-d NUMBER> | B<--debug=NUMBER>

Sets the -DCPSDebugLevel option value.

The default value is 0.

=item B<-T NUMBER> | B<--tdebug=NUMBER>

Sets the -DCPSTransportDebugLevel option value.

The default value is 0.

=item B<-R NUMBER> | B<--rdebug=NUMBER>

Sets the -DCPSDebugLevel option value for the repository process.

The default value is 0.

=item B<-T NUMBER> | B<--tdebug=NUMBER>

Sets the -DCPSTransportDebugLevel option value.

The default value is 0.

=item B<-f FILE> | B<--dfile=FILE>

Sets the -ORBLogFile option value.

The default value is 0.

=item B<-t NAME> | B<--transport=NAME>

Establishes the transport type to use for the current test execution.

Accepted values are:
  tcp       - use the SimpleTCP transport;
  udp       - use the udp transport;
  multicast - use the multicast transport;

The default value is 'tcp'.

=item B<-p FILE> | B<--priority=NUMBER>

Priority to assign to publications for the test.

The default value is 1.

=back

=head1 DESCRIPTION

This test verifies the TRANSPORT_PRIORITY QoS policy support in OpenDDS.
It does so by creating 2 publications at different priority levels and a
single subscriber.  The low priority publication sends samples with
increasing sequence numbers, until a timeout occurs on write, then the
high priority message is sent, indicating the last low priority samples
sequence number that was sent.  The subscriber then verifies that the
high priority sample was received before the low priority sample with
the indicated sequence number is received.

=head1 EXAMPLES

=over 8

=item B<./run_demo.pl -n>

=item B<./run_demo.pl -d 10 -T 4 -f test.log -t udp>

=item B<./run_demo.pl -x -t mc>

=item B<./run_test.pl -vd10T4Vt mc -p 8>

=back

=cut

__END__

