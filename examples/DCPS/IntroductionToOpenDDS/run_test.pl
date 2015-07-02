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

########################################################################
#
# Process the command line.
#
my $help;
my $man;
my $transportDebug;
my $debug;
my $debugFile;
my $nobit;
my $udp;

GetOptions( "help|?"        => \$help,
            "man"           => \$man,
            "debug|d=i"     => \$debug,
            "transport|t=i" => \$transportDebug,
            "logfile|f=s"   => \$debugFile,
            "udp|u"         => \$udp,
            "nobit|x"       => \$nobit,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;

my $status = 0;
my $failed = 0;

my $repo_ior     = PerlACE::LocalFile ("repo.ior");

my $tcp_ini    = PerlACE::LocalFile ("dds_tcp_conf.ini");
my $udp_ini    = PerlACE::LocalFile ("dds_udp_conf.ini");

# Change how test is configured according to which test we are.
my $common_opts    = " ";
   $common_opts   .= "-DCPSDebugLevel $debug " if $debug;
   $common_opts   .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;
   $common_opts   .= "-ORBLogFile $debugFile " if $debugFile;
my $repo_opts      = "-ORBEndpoint iiop://localhost:12345";
my $publisher_opts = "-DCPSConfigFile $tcp_ini ";
my $sub1_opts      = "-DCPSConfigFile $tcp_ini ";
my $sub2_opts      = "-DCPSConfigFile $tcp_ini ";

if( $udp) {
  $publisher_opts  = "-DCPSConfigFile $udp_ini ";
  $sub1_opts       = "-DCPSConfigFile $udp_ini ";
  $sub2_opts       = "-DCPSConfigFile $udp_ini ";
}

if( $nobit) {
  $common_opts .= "-DCPSBit 0 ";
}

my $repo_args        = "$common_opts $repo_opts ";
my $publisher_args   = "$common_opts $publisher_opts ";
my $subscriber1_args = "$common_opts $sub1_opts ";
my $subscriber2_args = "$common_opts $sub2_opts ";

my $REPO;
my $PUBLISHER;
my $SUBSCRIBER1;
my $SUBSCRIBER2;

$REPO        = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo", $repo_args);
$PUBLISHER   = PerlDDS::create_process ("publisher", $publisher_args);
$SUBSCRIBER1 = PerlDDS::create_process ("subscriber", $subscriber1_args);
$SUBSCRIBER2 = PerlDDS::create_process ("subscriber", $subscriber2_args);

unlink $repo_ior;

# Fire up the repository.
print $REPO->CommandLine(), "\n";
$REPO->Spawn ();
if (PerlACE::waitforfile_timed ($repo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for repository IOR file\n";
    $REPO->Kill ();
    exit 1;
}

# Fire up the publisher
print $PUBLISHER->CommandLine(), "\n";
$PUBLISHER->Spawn ();

# Fire up the subscribers
print $SUBSCRIBER1->CommandLine(), "\n";
$SUBSCRIBER1->Spawn ();

print $SUBSCRIBER2->CommandLine(), "\n";
$SUBSCRIBER2->Spawn ();

# Wait up to 5 minutes for test to complete.

$status = $SUBSCRIBER1->WaitKill (300);
if ($status != 0) {
    print STDERR "ERROR: Subscriber 1 returned $status\n";
}
$failed += $status;

$status = $SUBSCRIBER2->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: Subscriber 2 returned $status\n";
}
$failed += $status;

# And it can, in the worst case, take up to half a minute to shut down the rest.

$status = $PUBLISHER->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: Publisher returned $status\n";
}
$failed += $status;

$status = $REPO->TerminateWaitKill(5);
if ($status != 0) {
    print STDERR "ERROR: Repository returned $status\n";
}
$failed += $status;

# Report results.

if ($failed == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;

=head1 NAME

run_test.pl - Execute the tests

=head1 SYNOPSIS

./run_test.pl [options]

Options:

  -? | --help   brief help message

  --man         full documentation

  -x | --nobit  execute with Builtin Topics disabled

  -d NUMBER | --DCPSDebugLevel=NUMBER
                set the corresponding DCPS debug level

  -f FILENAME | --logfile=FILENAME
                set the logfile name

  -u | --udp    execute using UDP transport

=head1 OPTIONS

=over 8

=item B<-?> | B<--help>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<-x> | B<--nobit>

Execute the test with Builtin Topics disabled.

The default is to execute with Builtin Topic support enabled.

=item B<-d NUMBER> | B<--DCPSDebugLevel=NUMBER>

Sets the -DCPSDebugLevel ORB option value.

The default debug level is 0.

=item B<-u> | B<--udp>

Execute using the UDP transports.

The default is to execute the test using the TCP transports.

=back

=head1 DESCRIPTION

This script is a simple example demonstrating OpenDDS in action.

=head1 EXAMPLES

=over 8

=item B<./run_test.pl>

Executes the test with Builtin Topics enabled, TCP transports and
debugging disabled.

=item B<./run_test.pl -u -d10>

Executes the test with Builtin Topics enabled, UDP transports and
maximum level debugging enabled.

=item B<./run_test.pl -x>

Executes the test with TCP transports, debugging disabled and the Builtin
Topics disabled.

=back

=cut

__END__

