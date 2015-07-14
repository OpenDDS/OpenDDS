eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env qw( DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

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

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();
$test->{add_pending_timeout}=0;

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
if (!$bit) {
  $test->{nobits} = 1;
}

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
my $debugFile = PerlACE::LocalFile( $dFile) if $dFile;
my $iniFile   = PerlACE::LocalFile( $iniFile);

# Set $cFile above to bring in a service configurator configuration file.
my $confOpts = "";
$confOpts = "-ORBSvcConf $cFile " if $cFile;

my $commonOpts = new PerlACE::ConfigList->check_config('STATIC')?
                     "": $confOpts;
$commonOpts .= "-ORBVerboseLogging 1 " if $orbVerbose;
$commonOpts .= "-ORBLogFile $debugFile " if $debugFile;

my $repoOpts = "$commonOpts -o $repo_ior ";
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug "
             if $repoDebug and $transportDebug;
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;

my $appOpts = "$commonOpts -n $num_messages -d $data_size ";
$appOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
$appOpts .= "-DCPSDebugLevel $debug " if $debug;
$appOpts .= "-DCPSConfigFile $iniFile " if $iniFile;

my $subOpts = "$appOpts ";
$subOpts .= "-p $num_writers ";
$subOpts .= "-i $num_msgs_btwn_rec ";
$subOpts .= "-msi $num_samples ";
$subOpts .= "-mxs $num_samples ";
$subOpts .= "-c $copy_sample ";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap
#   (could be less than $num_messages but I am not sure of the limit).

foreach my $index (0..($num_readers-1)) {
  $test->process("subscriber$index", "subscriber", $subOpts);
}

my $pubOpts = "$appOpts ";
$pubOpts .= "-p 1 ";
$pubOpts .= "-r $num_readers ";
$pubOpts .= "-msi 1000 ";
$pubOpts .= "-mxs 1000 ";
#NOTE: above 1000 queue samples does not give any better performance.

foreach my $index (0..($num_writers-1)) {
  $test->process("publisher$index", "publisher", $pubOpts . " -i $index");
}

$test->setup_discovery($repoOpts);

foreach my $index (0..($num_readers-1)) {
  $test->start_process("subscriber$index");
}

foreach my $index (0..($num_writers-1)) {
  $test->start_process("publisher$index");
}

my $wait_to_kill = 200;
for (my $index = 0; $index < $num_writers; $index++) {
    if (!$test->stop_process($wait_to_kill, "publisher$index")) {
        $wait_to_kill = 0;
    }
}

for (my $index = 0; $index < $num_readers; $index++) {
    if (!$test->stop_process($wait_to_kill, "subscriber$index")) {
        $wait_to_kill = 0;
    }
}

# just cleaning up InfoRepo (if there)
my $status = $test->finish(20);

exit $status;

=head1 NAME

run_test.pl - Execute a TCPListener test.

=head1 SYNOPSIS

./run_test.pl [options]

Options:

  -? | --help            brief help message

  --man                  full documentation

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

