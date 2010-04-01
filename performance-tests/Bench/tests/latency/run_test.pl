eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Cross_Sync;

$| = 1;
my $status = 0;
my $bench_location = "$ENV{DDS_ROOT}/performance-tests/Bench";
my $trans_config_file = "$bench_location/etc/transport-tcp.ini";
my $pub_config_file = "$bench_location/tests/latency/p1-50.ini";
my $sub_config_file = "$bench_location/tests/latency/p2.ini";


if ($ARGV[0] eq 'udp') {
    $trans_config_file = "$bench_location/etc/transport-udp.ini ";
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


$CS = new PerlDDS::Cross_Sync (1, PerlACE::random_port(), PerlACE::random_port()
                        , $pub_config_file, $sub_config_file);
if (!$CS) {
    print "Crossplatform test pre-reqs not met. Skipping...\n";
    exit 0;
}

@test_configs = $CS->get_config_info();
$pub_config_file = @test_configs[0];
$sub_config_file = @test_configs[1];

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

unlink $dcpsrepo_ior;

if ($role == PerlDDS::Cross_Sync_Common::SERVER) {
    unlink $dcpsrepo_ior;
TODO: change to use standard ir 
DCPSInfoRepo -ORBSvcConf /tao_builds/fields_t/1.6a/DDS/build/gcc_d1o0/performance-tests/Bench/bin/../etc/svc.conf  -ORBListenEndpoints iiop://127.0.0.1:2809  -o /tao_builds/fields_t/1.6a/DDS/build/gcc_d1o0/performance-tests/Bench/tests/thru/tcp/repo.ior
    $DCPSREPO = PerlDDS::create_process
          ("$bench_location/bin/run_test",
           "-S -h iiop://$repo_host:$port1");

    print $DCPSREPO->CommandLine(). "\n";
    $DCPSREPO->Spawn ();
    if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
        print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
        $DCPSREPO->Kill ();
        exit 1;
    }
    unlink $dcpsrepo_ior;

    $Publisher = PerlDDS::create_process
          ("$bench_location/bin/run_test",
           "$common_args -s $pub_config_file");

    print $Publisher->CommandLine(). "\n";
    $Publisher->Spawn ();

    if ($CS->ready () == -1) {
        print STDERR "ERROR: subscriber failed to initialize.\n";
        $status = 1;
        $DCPSREPO->Kill ();
        $Publisher->Kill ();
        exit 1;
    }

    $PublisherResult = $Publisher->WaitKill (180);
    if ($PublisherResult != 0) {
        print STDERR "ERROR: publisher returned $PublisherResult \n";
        $status = 1;
    }

    $ir = $DCPSREPO->TerminateWaitKill(5);
    if ($ir != 0) {
        print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
        $status = 1;
    }
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

#  LocalWords:  eval
