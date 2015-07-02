eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

# Locate the project - *before* we use it to locate Perl modules or config files.
use FindBin;
my $projectRoot;
BEGIN { $projectRoot = "$FindBin::Bin/.."; }

# Find the current hostname
use Sys::Hostname;

# Use information from the environment.
use Env qw( @LD_LIBRARY_PATH
            @LIB_PATH
            @SHLIB_PATH
            @PATH
            $DDS_ROOT
            $ACE_ROOT);

# Locate the Perl modules
use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use lib "$projectRoot/bin";
use PerlDDS::Run_Test;

########################################
#
# Locate the link libraries
# PerlDDS::add_lib_path( "$projectRoot/lib");
#  NOTE: add_lib_path() does not appear to export the path to the
#        subprocess sufficiently for use by our subprocesses.
unshift @LD_LIBRARY_PATH, "$projectRoot/lib";
unshift @LIB_PATH,        "$projectRoot/lib";
unshift @SHLIB_PATH,      "$projectRoot/lib";

# I find this a particularly heinous setting.
if( $^O eq 'MSWin32') { unshift @PATH, "$projectRoot/lib"; }
#
########################################

# Locate the commands.
unshift @PATH, "$FindBin::Bin";
my( $repoCommand, $repoOpts) = &findCommand('DCPSInfoRepo') =~ /(\S+)\s+(.*)$/;

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

#
# Specific options.
#
my $duration;
my $startRepo;
my $startDds;
my $repoHost;
my $iniFile;
my $procspecs;
my $procfile;
my $procsubset;
my $collectStats;
my $confFile  = $projectRoot . "/etc/svc.conf";
my $statsOutputDecorator = "-P.log";

########################################################################
#
# Process the command line.
#
#   -v -V -? -d -T -R -x -f -C -O -h -i -s -F -p -S -P -t -g
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
            "collect|C=s"         => \$collectStats,
            "outputdecorator|O=s" => \$statsOutputDecorator,
            "repohost|h=s"        => \$repoHost,
            "inifile|i=s"         => \$iniFile,
            "commands|s=s"        => \$procspecs,
            "commandfile|F=s"     => \$procfile,
            "procsubset|p=s"      => \$procsubset,
            "startrepo|S"         => \$startRepo,
            "starttest|P"         => \$startDds,
            "duration|t=i"        => \$duration,
            "svcconf|g=s"         => \$confFile,

) or pod2usage( 0) ;
pod2usage( 1)             if $help or ($startDds and not ($procspecs or $procfile));
pod2usage( -verbose => 2) if $man;
#
########################################################################

# Extract the DDS commands.
my @commandList;
push @commandList, split( ',', $procspecs) if $procspecs;

if( $procfile) {
  # Slurp in any specified command listing file.
  die "Could not open command specifiction file $procfile: $!"
    if not open my $in, '<', "$procfile";
  my $contents = do { local $/; <$in> };
  close $in;

  # Support line continuations.
  $contents =~ s/\\\n//g;

  # Push the individual commands on the list, removing comments and blank
  # lines.
  push @commandList, grep { s/\#.*$//; $_ if not /^\s*$/; }
                     split( "\n", $contents);
}

$procsubset =~ s/^=//;
# Restrict the commands that are actually executed.
$procsubset =~ s/,/|^/g;
@commandList = grep(/^$procsubset/, @commandList);

# Locate the test commands.
my @commands = map &findCommand($_), @commandList;

# Allow 'all' to be specified for statistics collection.
$collectStats = "psn" if $collectStats =~ /al?l?/;

# Verbosity.
print "Commands will be printed only and not executed.\n"
                                          if $verbose and $noaction;
print "Repository will be started\n"      if $verbose and $startRepo;
print "Repository is located at: $repoCommand\n"
                                          if $verbose;
if( $verbose and $startDds) {
  print "DDS process(es) will be started:\n";
  for my $command (@commands) {
    print "  $command\n";
  }
}
print "DDS will execute for at least $duration seconds before terminating\n"
                                          if $verbose and $duration;
print "DDS will execute continuously and not terminate\n"
                                          if $verbose and not $duration;
print "ProjectRoot==$projectRoot\n"       if $verbose;
print "RepoHost==$repoHost\n"             if $verbose;
print scalar(@commmands) . " procspecs==" . join(', ', @commmands) . "\n"
                                          if $verbose;
print "Process Statistics will be collected.\n"
                                          if $verbose and $collectStats =~ /p/;
print "System Statistics will be collected.\n"
                                          if $verbose and $collectStats =~ /s/;
print "Network Statistics will be collected.\n"
                                          if $verbose and $collectStats =~ /n/;

print "Debug==$debug\n"                   if $verbose and $debug;
print "RepoDebug==$repoDebug\n"           if $verbose and $repoDebug;
print "TransportDebug==$transportDebug\n" if $verbose and $transportDebug;
print "DebugFile==$dFile\n"               if $verbose and $dFile;
print "VerboseLogging==$orbVerbose\n"     if $verbose and $orbVerbose;

# Kill processes after specified duration if they do not terminate independently.
my $killDelay = 30;

# Files.
my $repo_ior  = PerlACE::LocalFile("repo.ior");
my $debugFile;
   $debugFile = PerlACE::LocalFile( $dFile) if $dFile;

# Clean out leftovers.
unlink $repo_ior if $startRepo;
unlink $debugFile if $debugFile;

my $verboseDebug;
$verboseDebug = "-ORBVerboseLogging 1 " if $orbVerbose;

my $confOpts = "";
$confOpts = "-ORBSvcConf $confFile " if -r $confFile;
my $common_opts = new PerlACE::ConfigList->check_config ('STATIC') ? '' : $confOpts;
$common_opts .= $verboseDebug if $verboseDebug;
$common_opts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;

# Establish process arguments.

my $ddsDebug;
$ddsDebug  = $debug if $debug;

my $repoOpts = "$common_opts ";
$repoOpts .= "-ORBListenEndpoints iiop://$repoHost " if $repoHost;
$repoOpts .= "-DCPSDebugLevel $repoDebug " if $repoDebug;
$repoOpts .= "-ORBLogFile $debugFile " if $debugFile;

my $index = 0;
my $ddsOpts = "$common_opts ";
$ddsOpts .= "-DCPSConfigFile $iniFile " if $iniFile;
$ddsOpts .= "-DCPSDebugLevel $ddsDebug " if $ddsDebug;
$ddsOpts .= "-ORBLogFile $debugFile " if $debugFile;
$ddsOpts .= "-DCPSInfoRepo corbaloc:iiop:$repoHost/DCPSInfoRepo " if $repoHost;

# Define the processes.
my @PROCESSES;
my $repoArgs;
if( $startRepo) {
  $repoArgs = "$repoOpts -o $repo_ior ";
  if( PerlACE::is_vxworks_test()) {
    push @PROCESSES, new PerlACE::ProcessVX( $repoCommand, $repoArgs);
  } else {
    push @PROCESSES, new PerlACE::Process( $repoCommand, $repoArgs);
  }
}

if( $startDds) {
  map {
    my( $ddsCommand, $ddsArgs) = $_ =~ /^(\S+)\s+(.*)/;
    $ddsCommand = $_ if not $ddsCommand;
    $ddsArgs .= " $ddsOpts";
    if( PerlACE::is_vxworks_test()) {
      push @PROCESSES, new PerlACE::ProcessVX( $ddsCommand, $ddsArgs);
    } else {
      push @PROCESSES, new PerlACE::Process( $ddsCommand, $ddsArgs);
    }
  } @commands;
}

if( $noaction) {
  # Be verbose.
  map { print $_->CommandLine() . "\n"; } @PROCESSES;
  exit;
}

# Manage statistics collection processes.
my @COLLECTORS;
my @HANDLES;
&runCollector( "system", $$)  if $collectStats =~ /s/;

# Interrupts kill all the child processes as well as ourselves.
sub interrupt_handler {
  map {
    $status = $_->Kill();
  } reverse @PROCESSES;
  kill 2, @COLLECTORS;
  exit;
}
$SIG{'INT'} = 'interrupt_handler';

# Start and stop the test processes.
map {
  print "\nDDS PROCESS\n";
  print $_->CommandLine() . "\n";
  $_->Spawn();
  &runCollector( "process", $_->{PROCESS}) if $collectStats =~ /p/;
  &runCollector( "network", $_->{PROCESS}) if $collectStats =~ /n/;
  if( $startRepo and PerlACE::waitforfile_timed( $repo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for repository IOR file $repo_ior.\n";
    exit 1;
  }
} @PROCESSES;

if( $duration) {
  # Wait for the duration, then kill the processing.
  sleep $duration;

} else {
  # block forever if no duration was specified.
  WaitForUserInput();
}

# Terminate in reverse order from starting order.
my $numProcesses = scalar(@PROCESSES);
map {
  print "Terminating DDS process.\n" if $verbose;
  $status = $_->TerminateWaitKill( $killDelay);
  $killDelay = 5;

  if( $status != 0) {
    print STDERR "ERROR: Process returned $status\n";
    ++$failed;
  }
} reverse @PROCESSES;

# Clean up after the repository.
unlink $repo_ior if $startRepo;

# Terminate the statistics collectors.
kill 2, @COLLECTORS;

# Report results.
if( $failed == 0) {
  print "test PASSED.\n";

} else {
  print STDERR "test FAILED.\n";
}

exit $failed;

# Block until terminal input is received.
sub WaitForUserInput {
  print "\n\nNo Duration specified.  Hit Enter to end the processes "
      . "and finish the test.\n\n";
  my $userinput = <STDIN>;
}

#
# Search the environments command search path for a command to execute.
# The PerlACE::Process needs to have a fully located command to operate
# correctly, and does not honor the environment PATH.
#
# This is the equivalent of "return `which $command`;", but cross
# platform.  I hope.
#
# Any arguments passed as part of the command are stripped out for the
# search and then added back into the return value.
#
sub findCommand {
  my( $command, $args) = /(\S+)\s+(.*)$/;
  $command = shift if not $command;
  $command =~ s/\s*$//;
  if( $command =~ m!^/!) {
    return "$command $args" if -x "$command" || -x "${command}.exe";

  } else {
    foreach my $location (@PATH) {
      return "$location/$command $args" if -x "$location/$command" || -x "$location/${command}.exe";
    }
  }
  die "Unable to locate command: $command for execution.";
}

#
# Decorate the statistics gathering filename with user specified
# information.  This can include hostname, PID, a timestamp, and other
# static information, such as an extension.
#
# The decorator string is passed in and all instances of 'P' are replaced
# with the supplied PID value, all instances of 'H' are replaced with a
# derived hostname value, and all instances of 'T' are replaced with a
# derived timestamp.  This is a simple substitution and no escapes are
# allowed.
#
# A filename is formed by appending the suitably modified decorator to
# the base.
#
sub statsFilename {
  my ($base, $decorator, $pid) = @_;

  $decorator =~/T/ && do {
    my @now = localtime ;
    my $timestamp = sprintf "%04d%02d%02d%02d%02d%02d",
                      (1900+$now[5]), (1+$now[4]), $now[3],
                      $now[2], $now[1], $now[0] ;
    $decorator =~ s/T/$timestamp/;
  };

  $decorator =~/P/ && do {
    $decorator =~ s/P/$pid/;
  };

  # Hostname is last since it might contain a 'P' or a 'T'.
  $decorator =~/H/ && do {
    my $host  = hostname;
    $decorator =~ s/H/$host/;
  };

  return "$base$decorator";
}

#
# Statistics gathering commands by system type
#
# The collection processing will start the command /once/, then capture
# the output and process it line by line.  As this script is terminating,
# it will send an INT (3) signal to the process to stop it.
#
# If a system type does not honor the commands as written, add a stanza
# for that system type and modify the commands to be appropriate for that
# environment.
#
sub collectionCommand {
  my ( $type, $params) = @_;
  SWITCH:{
    $^O =~ /MSWin32/ && do {
      return undef;
    };
    $^O =~ /cygwin/ && do {
      return undef;
    };
    $^O =~ /VMS/ && do {
      return undef;
    };
    $^O =~ /solaris/ && do {
      return "vmstat 1"  if $type eq "system";
      return undef       if $type eq "network";
      return "top -bd 1" if $type eq "process"; # This still may not work.
    };

    # Handle generic Unix types here.  Assume they have modern command
    # forms.  If not, add a stanza to handle them specifically above.
    return "vmstat 1"             if $type eq "system";
    return "netstat -anpc --tcp"  if $type eq "network";
    return "top -bd 1 -p $params" if $type eq "process";
  }
  return undef;
}

#
# Statistics gathering filters by system type.  This routine returns a
# closure that will be executed for each line of output from the
# statistics command.  Currently the optional parameters available for
# use in the closures include the PID of the process being monitored.
#
sub collectionFilter {
  my ( $type, $params) = @_;
  OSSWITCH:{
    $^O =~ /MSWin32/ && do { last OSSWITCH; };
    $^O =~ /cygwin/  && do { last OSSWITCH; };
    $^O =~ /VMS/     && do { last OSSWITCH; };
    $^O =~ /solaris/ && do {
      if(    $type eq 'process') { return &defaultFilter(); }
      elsif( $type eq 'system')  { return &defaultFilter(); }
      elsif( $type eq 'network') { return &defaultFilter(); }
    };
  }

  # All other system types.
  if(    $type eq 'process') { return &defaultPidFilter( $params); }
  elsif( $type eq 'system')  { return &defaultFilter(); }
  elsif( $type eq 'network') { return &defaultPidFilter( $params); }
}

# Default filter timestamps and prints all lines.
sub defaultFilter {
  # Returns a closure.
  return sub {
    my ( $input, $output) = @_;
    print $output localtime() . ": $input\n";
  };
}

# Default PID filter timestamps and prints lines containing the PID.
sub defaultPidFilter {
  # Returns a closure.
  my ( $pid) = @_;
  return sub {
    my ( $input, $output) = @_;
    print $output localtime() . ": $input\n" if $input =~ /$pid/;
  };
}

# Start and attach to a statistic collector.
sub runCollector {
  my ( $type, $pid) = @_;
  return if not $type or not $pid;

  my $command  = &collectionCommand( $type, "$pid");
  return if not $command;

  my $filename = &statsFilename( $type, $statsOutputDecorator, $pid);
  my $action   = &collectionFilter( $type, $pid);

  # Start a sub-process to collect statistics.
  my $child = open( my $handle, "-|");
  if( not defined($child)) { # Broken
    warn "Unable to start statistics collection: $!";

  } elsif( $child) { # Parent
    # Obtain the statistics process Id to terminate processing with.
    my $cmd = <$handle>;
    chomp $cmd;
    push @COLLECTORS, $cmd; # For terminating.
    push @HANDLES, $handle; # For process scope.
    print "Starting to write statistics to <$filename> "
        . "from process <$cmd> running <$command>.\n";

  } else { # Child
    # Prevent issues with test process management in the child.  The
    # managed processes are managed only by the parent process.
    map $_->{RUNNING} = 0, @PROCESSES;

    # Start collecting using the provided command as input.
    my $cmd = open( STATSIN, "$command |")
      or die "Failed to start statistics gathering command <$command>: $!";

    # Send the command PID to the parent process.
    print "$cmd\n";

    # Open and unbuffer the output.
    open( my $output, "> $filename")
      or die "Failed to open statistics output file <$filename>: $!";
    select $output; $|=1;

    # Process all of the input.
    while(<STATSIN>) {
      s/\s+$//; # Elide trailing whitespace; 'chomp' doesn't work quite right.
      $action->( "$_", $output);
    }

    # Done collecting.
    close $output;
    exit;
  }
}

=head1 NAME

run_dds.pl - Execute one or more DDS applications

=head1 SYNOPSIS

 run_dds.pl [options]

=head1 OPTIONS

=over 8

=item B<-?> | B<--help>

Print a brief help message and exits.

=item B<--man>

Prints this manual page and exits.

=item B<-x> | B<--noaction>

Print the commands that would be executed with the current set of command
line options and exit without starting any processes.

=item B<-v> | B<--verbose>

Print additional information while executing.

=item B<-V> | B<--ORBVerboseLogging=NUMBER>

Sets the -ORBVerboseLogging option to NUMBER.

The default value is 0.

The value is set to 1 if any non-zero value is specified.  A value of
zero will omit the ORBVerboseLogging specification from the process
command line.

=item B<-d NUMBER> | B<--debug=NUMBER>

Sets the -DCPSDebugLevel option value.  A value of 0 will omit the
DCPSDebugLevel specification from the process command line.

The default value is 0.

=item B<-T NUMBER> | B<--tdebug=NUMBER>

Sets the -DCPSTransportDebugLevel option value.  A value of 0 will omit
the DCPSTransportDebugLevel specification from the process command line.

The default value is 0.

=item B<-R NUMBER> | B<--rdebug=NUMBER>

Sets the -DCPSDebugLevel option value for the repository process.  A
value of 0 will omit the DCPSDebugLevel specification from the repository
process command line.

The default value is 0.

=item B<-t NUMBER> | B<--duration=NUMBER>

Limits the execution time of the test.  If not specified, then any test
or repository process that is started will execute until the script is
interrupted.

The default value is unspecified.

=item B<-S> | B<--startrepo>

Causes a repository process to be started.  Only a single repository
process will be invoked by an invocation.

The default value is to not start a repository process.

=item B<-P> | B<--starttest>

Causes one or more application processes to be started.  One process will be
started for each command specified by the B<-s> option.

The default value is to not start an application.

=item B<-f FILE> | B<--dfile=FILE>

Sets the -ORBLogFile option value.  A value of unspecified will result in
the ORBLogFile specification being omitted from the process command line.

The default value is unspecified.

=item B<-p LIST> | B<--procsubset=LIST>

List of patterns that will be used to select a subset of the commands
to execute from the command list.

The default value is unspecified, and all commands will be executed.

=item B<-C TYPES> | B<--collect=TYPES>

Starts statistics collection for the specified TYPES.  TYPES may contain
one or more of the values: C<n>, C<s>, and C<p>, or the collective
specification C<all>.
The output for any given statistic is placed in a file named as specified
by the B<-O FORMAT> command line option.  The default is to a file with a
basename of the TYPE and a decorator of C<-<pidE<gt>.log>.

=over 8

=item B<n>

Causes network statistics to be gathered.  A C<netstat -ntpc> command is
started for each test process and the output filtered by its process Id.

=item B<s>

Causes system statistics to be gathered.  A single C<vmstat 1> command is
started.  If the PID value is specified as part of the statistic
filename, the PID value for the test script process is used.

=item B<p>

Causes process statistics to be gathered.  A C<top -bd 1 -p {pid}>
command is started for each test process and the output filtered by its
process Id.

=item B<all>

Causes all statistics described above to be gathered.  This is a synonym
for I<nsp>

=back

The default is unspecified.  This results in no statistics being
collected.

=item B<-O FORMAT> | B<--outputdecorator=FORMAT>

Establishes the format of the statistics gathering output file names.
The format is specified as a string with simple substitution performed to
replace some characters with other information.  This is a simple
substitution and no escapes are allowed.

Statistics output filenames are formed starting with the type of
statistic being gathered concatenated with the decorater defined here,
expanded with the formatting information at the time of execution.  The
types of statistics currently include C<system>, C<process>, and
C<network>.

The characters replaced in the format specification are:

=over 8

=item B<P>

replaced with the process PID for being monitored.

=item B<T>

replaced with a timestamp that includes a 4 digit year, 2 digit month, 2
digit day of month, 2 digit hour, 2 digit minute, and 2 digit second.
The timestamp is taken near the time the statistics collection is started.

=item B<H>

replaced by the hostname of the machine executing the script.

=back

The default is "-P.log" which results in output files with names similar
to C<system-5436.log>.

=item B<-h FQDN> | B<--repohost=FQDN>

This is the fully qualified domain name and port where the OpenDDS
repository may be found.

The default value is 'localhost:2112'.

=item B<-i FILE> | B<--inifile=FILE>

OpenDDS configuration filename.  This defines the configuration
information for the OpenDDS service.

The default is to use the file located in the 'etc' directory relative
from the project root (the parent directory of the directory where the
command was executed from) with filename 'transport.ini'.

=item B<-g FILE> | B<--svcconf=FILE>

ACE service configuration filename.  This defines the configuration
information for the ACE service configurator.

The default is to use the file located in the 'etc' directory relative
from the project root (the parent directory of the directory where the
command was executed from) with filename 'svc.conf'.

=item B<-s SPECLIST> | B<--commands=SPECLIST>

Application process definitions.  Each test process to be executed should
be in the list.  The list consists of applications and argument lists
as if they were specified on the command line.  The command specified
should be reachable via the command path.  The individual commands are
separated by commas.  Any spaces or other special characters in the
commands should be escaped.

The default is not specified.  This will result in no application
being started if the B<-P> argument is given.

=item B<-F FILE> | B<--commandfile=FILE>

Application process definition file.  The file should contain commands,
one per line.  Each command should be complete and specified as if it
were being executed from the command line.  Each command should be
reachable via the command path.

Perl style comments are allowed: from sharp ('#') to the
end of the current line.  Blank lines are ignored.  Line continuation
is allowed using a backslash immediately prior to the end of line.
The newline is removed, so whitespace needs to be added explicitly
between the end of the previous line and the beginning of the next line
if it is needed.

Commands will be started in the order they appear in the file, and
terminated in reverse order.  If the repository was specified to start as
well, that is started prior to any commands and terminated after all
commands are terminated.

There is no default, and no commands will be taken from a file if this
argument is not specified.

=back

=head1 DESCRIPTION

This script manages execution of the applications needed for the
Correlation Module processing.
The processes used by the Module include the OpenDDS specific
repository process and the Module specific applications.  The repository
executable is the standard OpenDDS C<DCPSInfoRepo> program.  The Module
applications provide the
ability to start multiple publications and / or subscriptions within a
single process.

It is possible to start any number of applications at once.
A separate application will be started for each command specification.
The same command can be included more than once to start
separate application instances of the same type.

Applications started by this script will either execute until terminated
by the user (no duration specified) or until a specified duration has
elapsed.  The script will wait 60 seconds beyond this time and then
terminate the process by force.

This script will establish the environment for the executable processes
by adding the project library directory to the runtime library search path.

=head1 EXAMPLES

=over 8

=item B<bin/run_dds -vx -P -s "storage -ORBLogFile storage.log">

prints the commands that would be invoked when starting a single
application with the C<storage> test command.  This will also be verbose
during setup processing.

=item B<bin/run_dds -x -t 120 -P -s "datasource -f testdata","interpolate -i 0","interpolate -i 1">

prints the commands that would be invoked when starting 3 applications:
two using the interpolate application and one using the datasource
application.  The execution would be scheduled to last for 2 minutes.

=item B<bin/run_dds -x -t 120 -P -F testcommands>

if the file C<testcommands> contains the following lines:

=over 8

 # Send data from here.
 datasource -f testdata

 # Process data in these.
 interpolate -i 0
 interpolate -i 1

=back

prints the commands that would be invoked when starting 3 applications:
two using the interpolate application and one using the datasource
application.  The execution would be scheduled to last for 2 minutes.

This is the same example as the previous, but with the commands specified
in a separate file rather than directly on the command line.

=item B<bin/run_dds -vd10T4VCall -O-H-T-P.log -s "correlate -i 0 -t 5" -h machine.domain.com:2112>

starts an application using the C<correlate> command and
expecting to connect to the repository at C<machine.domain.com:2112>.  It
will run with C<DCPSDebugLevel> of 10, C<DCPSTransportDebugLevel> of 4,
C<ORBVerboseLogging> enabled and both the script and test process will
execute in verbose mode.  The log output will be directed to the standard
output.

All statistics - system, network, and process - will be gathered.
Statistics will be placed in files named using the statistic type, the
hostname, a timestamp, and the PID of the monitored process.  These
filenames will appear as: C<system-<hostE<gt>-<timeE<gt>-<pidE<gt>.log>,
C<process-<hostE<gt>-<timeE<gt>-<pidE<gt>.log>, and
C<network-<hostE<gt>-<timeE<gt>-<pidE<gt>.log>.

=back

=cut

__END__

# vim: filetype=perl

