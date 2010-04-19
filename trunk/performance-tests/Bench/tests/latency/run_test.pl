eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

=head1 NAME

run_test.pl - run one side of a latency cross host test

$Id$

=head1 SYNOPSIS

  run_test.pl <transport> <message size>

=head1 DESCRIPTION

This script runs one side of the latency test for a cross host testing.  The 
script needs to be run on each of the two hosts involved in the test using 
the same parameters on each host.

The test consists of two halves, an originating (server) side and a reflecting 
(client) side. The servers involved in the test are are stored in 
test_list.txt file in test-host groupings.  The grouping consists of an
ID, client host, and server host.  The script identifies the host's behavior 
by identifying the test group ID and the local host's name.  The test group 
ID is identified using the environement variable CROSS_GRP.

The server (originiating) side starts the DCPSInfoRepo for the test.

The transport has to be one of the following values:
=over 8
=item tcp
SimpleTCP
=item udp
SimpleUDP
=item multibe
multicast (Best Effort)
=item multire
multicast (Reliable)
=back

Supported message sizes are B<50> B<100> B<250> B<500> B<1000>
 B<2500> B<5000> B<8000> B<16000> B<32000>


=head1 EXAMPLE

  run the same command on both hosts:

  run_test.pl tcp 1000

  run_test.pl multibe 50

=cut

use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Cross_Sync;


sub configure_transport_file {
  my $currfile = shift;
  my $myhostname = shift;
  my $newfile = $currfile . ".configured";
  open(NEWCONFIGFILE, "$newfile") and close (NEWCONFIGFILE) && return $newfile;
  open(CONFIGFILE, "$currfile") or die "Failed to open the transport file $currfile\n";
  open(NEWCONFIGFILE, ">$newfile") or die "Failed to open the transport file $newfile for modification\n";

  while (<CONFIGFILE>) {
    s/\<%HOSTNAME%\>/$myhostname/g;
    print NEWCONFIGFILE "$_\n";
  }
  close NEWCONFIGFILE;
  close CONFIGFILE;
  return $newfile;
}


$| = 1;
my $status = 0;
my $bench_location = "$ENV{DDS_ROOT}/performance-tests/Bench";
my $trans_config_file = "$bench_location/etc/transport-tcp.ini";
my $pub_config_file = "$bench_location/tests/latency/p1-50.ini";
my $sub_config_file = "$bench_location/tests/latency/p2.ini";


if ($ARGV[1] == 100) {
    $pub_config_file = "$bench_location/tests/latency/p1-100.ini ";
}
elsif ($ARGV[1] == 250) {
    $pub_config_file = "$bench_location/tests/latency/p1-250.ini ";
}
elsif ($ARGV[1] == 500) {
    $pub_config_file = "$bench_location/tests/latency/p1-500.ini ";
}
elsif ($ARGV[1] == 1000) {
    $pub_config_file = "$bench_location/tests/latency/p1-1000.ini ";
}
elsif ($ARGV[1] == 2500) {
    $pub_config_file = "$bench_location/tests/latency/p1-2500.ini ";
}
elsif ($ARGV[1] == 5000) {
    $pub_config_file = "$bench_location/tests/latency/p1-5000.ini ";
}
elsif ($ARGV[1] == 8000) {
    $pub_config_file = "$bench_location/tests/latency/p1-8000.ini ";
}
elsif ($ARGV[1] == 16000) {
    $pub_config_file = "$bench_location/tests/latency/p1-16000.ini ";
}
elsif ($ARGV[1] == 32000) {
    $pub_config_file = "$bench_location/tests/latency/p1-32000.ini ";
}


if ($ARGV[0] eq 'udp') {
    # trans_config_file assigned after determining cross host identity
    mkdir "udp", 0777 unless -d "udp";
    chdir "udp";
}
elsif ($ARGV[0] eq 'multibe') {
    $trans_config_file = "$bench_location/etc/transport-multi-be.ini ";
    mkdir "multibe", 0777 unless -d "multibe";
    chdir "multibe";
}
elsif ($ARGV[0] eq 'multire') {
    $trans_config_file = "$bench_location/etc/transport-multi-rel.ini ";
    mkdir "multire", 0777 unless -d "multire";
    chdir "multire";
}
elsif ($ARGV[0] eq 'tcp') {
    mkdir "tcp", 0777 unless -d "tcp";
    chdir "tcp";
}
else {
    print "Unknown transport. Skipping...\n";
    exit 0;
}


$CS = new PerlDDS::Cross_Sync (1, PerlACE::random_port(), PerlACE::random_port()
                        , $pub_config_file, $sub_config_file, "../test_list.txt");
if (!$CS) {
    print "Crossplatform test pre-reqs not met. Skipping...\n";
    exit 0;
}

if ($ARGV[0] eq 'udp') {
    $trans_config_file = configure_transport_file ("$bench_location/etc/transport-udp.ini", $CS->self());
}

my $role = $CS->wait();
if ($role == -1) {
    print "ERROR: Test pre-reqs not met.\n";
    exit -1;
}

@ports = $CS->boot_ports ();
my($port1) = 10001 + @ports[0];
my $dcpsrepo_ior = "repo.ior";
my $repo_host;
if ($role == PerlDDS::Cross_Sync_Common::SERVER) {
    $repo_host = $CS->self();
} else {
    $repo_host = $CS->peer();
}
my $common_args = "-P -t 120 -h $repo_host:$port1 -i $trans_config_file ";

if ($role == PerlDDS::Cross_Sync_Common::SERVER) {
    unlink $dcpsrepo_ior;

    $Publisher = PerlDDS::create_process
          ("$bench_location/bin/run_test",
           "$common_args -S -s $pub_config_file");

    print $Publisher->CommandLine(). "\n";
    $Publisher->Spawn ();

    if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
        print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
        $Publisher->Kill ();
        exit 1;
    }

    if ($CS->ready () == -1) {
        print STDERR "ERROR: subscriber failed to initialize.\n";
        $status = 1;
        $Publisher->Kill ();
        exit 1;
    }

    $PublisherResult = $Publisher->WaitKill (180);
    if ($PublisherResult != 0) {
        print STDERR "ERROR: publisher returned $PublisherResult \n";
        $status = 1;
    }

    unlink $dcpsrepo_ior;
} else {
    $Subscriber = PerlDDS::create_process
          ("$bench_location/bin/run_test",
           "$common_args -s $sub_config_file");

    print $Subscriber->CommandLine(). "\n";
    $Subscriber->Spawn ();

    $SubscriberResult = $Subscriber->WaitKill (180);
    if ($SubscriberResult != 0) {
        print STDERR "ERROR: subscriber returned $SubscriberResult \n";
        $status = 1;
    }
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}



exit $status;

