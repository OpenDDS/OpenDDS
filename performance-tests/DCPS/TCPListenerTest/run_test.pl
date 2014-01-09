eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env qw( DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

PerlDDS::add_lib_path('../TypeNoKeyBounded');

use Getopt::Long qw( :config bundling) ;
use Pod::Usage ;

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

# single reader with single instances test
my $num_messages=500;
my $data_size=13;
my $num_writers=2;
my $num_readers=3;
my $num_msgs_btwn_rec= 1;# 20;
my $copy_sample=0;
my $cFile; # Service configurator file -- none.
my $iniFile="conf.ini"; # DCPS initialization file.

# default bit to off
my $bit;
my $repo_bit_conf = "-NOBITS ";
my $app_bit_conf = "-DCPSBit 0 ";

########################################################################
#
# Process the command line.
#
GetOptions( "verbose!"            => \$verbose,
            "v"                   => \$verbose,
            "ORBVerboseLogging|V" => \$orbVerbose,
            "help|?"              => \$help,
            "man"                 => \$man,
            "debug|d=i"           => \$debug,
            "tdebug|T=i"          => \$transportDebug,
            "rdebug|R=i"          => \$repoDebug,
            "noaction|x"          => \$noaction,
            "dfile|f=s"           => \$dFile,
            "bit|b"               => \$bit,
            "zerocopy|c"          => \$copy_sample,
            "pubs|p=i"            => \$num_writers,
            "subs|s=i"            => \$num_readers,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;
#
########################################################################

# Un-disable (?) the built in topics.
$repo_bit_conf = undef if $bit;
$app_bit_conf  = undef if $bit;

# Verbosity.
print "Debug==$debug\n"                   if $verbose and $debug;
print "RepoDebug==$repoDebug\n"           if $verbose and $repoDebug;
print "TransportDebug==$transportDebug\n" if $verbose and $transportDebug;
print "DebugFile==$dFile\n"               if $verbose and $dFile;
print "VerboseLogging==enabled\n"         if $verbose and $orbVerbose;
print "BuiltinTopics==enabled\n"          if $verbose and $bit;
print "ZeroCopy==enabled\n"               if $verbose and $copy_sample;
print "Publications==$num_writers\n"      if $verbose;
print "Subscriptions==$num_readers\n"     if $verbose;

# need $num_msgs_btwn_rec unread samples plus 20 for good measure
# (possibly allocated by not yet queue by the transport because of greedy read).
my $num_samples=$num_msgs_btwn_rec + 20;

# Files.
my $repo_ior  = PerlACE::LocalFile("repo.ior");
my $debugFile;
   $debugFile = PerlACE::LocalFile( $dFile) if $dFile;
my $iniFile   = PerlACE::LocalFile( $iniFile);

# Clean out leftovers.
unlink $repo_ior;
# unlink $debugFile if $debugFile;  # This will accumulate/append.

# Set $cFile above to bring in a service configurator configuration file.
my $confOpts = "";
$confOpts = "-ORBSvcConf $cFile " if $cFile;

my $commonOpts = new PerlACE::ConfigList->check_config('STATIC')?
                     "": $confOpts;
$commonOpts .= "-ORBVerboseLogging 1 " if $orbVerbose;
$commonOpts .= "-ORBLogFile $debugFile " if $debugFile;

my $repoOpts = "$commonOpts $repo_bit_conf -o $repo_ior ";
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug "
             if $repoDebug and $transportDebug;
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                        $repoOpts);

my $appOpts = "$commonOpts -n $num_messages -d $data_size ";
$appOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$appOpts .= "-DCPSDebugLevel $debug " if $debug;
$appOpts .= "-DCPSConfigFile $iniFile " if $iniFile;
$appOpts .= "$app_bit_conf " if $app_bit_conf;

my $subOpts = "$appOpts ";
$subOpts .= "-p $num_writers ";
$subOpts .= "-i $num_msgs_btwn_rec ";
$subOpts .= "-msi $num_samples ";
$subOpts .= "-mxs $num_samples ";
$subOpts .= "-c $copy_sample ";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap
#   (could be less than $num_messages but I am not sure of the limit).

my @subs = map {
  PerlDDS::create_process("subscriber", $subOpts);
} (0..($num_readers-1));

my $pubOpts = "$appOpts ";
$pubOpts .= "-p 1 ";
$pubOpts .= "-r $num_readers ";
$pubOpts .= "-msi 1000 ";
$pubOpts .= "-mxs 1000 ";
#NOTE: above 1000 queue samples does not give any better performance.

my @pubs = map {
  PerlDDS::create_process("publisher", $pubOpts . "-i $_");
} (0..($num_writers-1));

if( $noaction) {
  print $DCPSREPO->CommandLine() . "\n" if $verbose;
  map { print $_->CommandLine() . "\n"} @subs if $verbose;
  map { print $_->CommandLine() . "\n"} @pubs if $verbose;
  exit;
}

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($repo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

map {
  print $_->CommandLine() . "\n";
  $_->Spawn();
} @subs;

map {
  print $_->CommandLine() . "\n";
  $_->Spawn();
} @pubs;

my $wait_to_kill = 200;
for (my $i = 0; $i < $num_writers; $i++) {
    my $PubResult = $pubs[$i]->WaitKill ($wait_to_kill);
    if ($PubResult != 0) {
        print STDERR "ERROR: publisher $i returned $PubResult \n";
        $status = 1;
        $wait_to_kill = 0;
    }
}

for (my $i = 0; $i < $num_readers; $i++) {
    my $SubResult = $subs[$i]->WaitKill ($wait_to_kill);
    if ($SubResult != 0) {
        print STDERR "ERROR: subscriber $i returned $SubResult \n";
        $status = 1;
        $wait_to_kill = 0;
    }
}

my $ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}


exit $status;

=head1 NAME

run_test.pl - Execute a TCPListener test.

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

  -b | --bit
                         enable the built-in topics

  -c | --zerocopy
                         enable use of zero copy reading

  -p NUMBER | --pubs=NUMBER
                         number of publication processes to start
                         default is 2

  -s NUMBER | --subs=NUMBER
                         number of subscription processes to start
                         default is 3

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

=item B<-b> | B<--bit>

Enables the use of the Builtin Topics for both the test processes as well
as the repository process.

The default value is to execute with the Builtin Topics disabled.

=item B<-c> | B<--zerocopy>

Enable the use of zero copy reading in the subscription listeners.

The default value is to use standard copy out semantics.

=item B<-p NUMBER> | B<--pubs=NUMBER>

Number of publication test processes to start.

The default value is 2.

=item B<-s NUMBER> | B<--subs=NUMBER>

Number of subscription test processes to start.

The default value is 3.

=back

=head1 DESCRIPTION

This test executes mutliple publication to multiple subscription testing
to allow performance of the TCP transport to be evaluated.

=head1 EXAMPLES

=over 8

=item B<./run_demo.pl>

=item B<./run_demo.pl -b -p 5 -s 4>

=item B<./run_demo.pl -vx>

=item B<./run_test.pl -vd10T4V -c -p 8 -s 4>

=back

=cut

