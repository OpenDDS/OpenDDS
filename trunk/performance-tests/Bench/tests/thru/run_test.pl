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
my $total_status = 0;
my $bench_location = "$ENV{DDS_ROOT}/performance-tests/Bench";
my $transport_type = $ARGV[0];
my $trans_config_file;
my $pub_config_file;
my $sub_config_file;
my $starting_test_number = 1;
my $ending_test_number = 14;
my $current_dir = getcwd;

my @pub_be_config_files = ("$bench_location/tests/thru/bidir-1sub-be-80.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-320.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-720.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-1280.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-2000.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-160r.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-240r.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-320r.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-400r.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-160s.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-240s.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-320s.ini",
                           "$bench_location/tests/thru/bidir-1sub-be-400s.ini");

my @pub_rel_config_files = ("$bench_location/tests/thru/bidir-1sub-rel-80.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-320.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-720.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-1280.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-2000.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-160r.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-240r.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-320r.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-400r.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-160s.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-240s.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-320s.ini",
                            "$bench_location/tests/thru/bidir-1sub-rel-400s.ini");

if ($transport_type eq 'udp') {
    ## UDP transport file is set after the hostname used in the cross host
    ## testing is determined.
    $sub_config_file = "$bench_location/tests/thru/bidir-remote-be.ini";
    mkdir "udp", 0777 unless -d "udp";
    chdir "udp";
}
elsif ($transport_type eq 'multibe') {
    $trans_config_file = "$bench_location/etc/transport-multi-be.ini ";
    $sub_config_file = "$bench_location/tests/thru/bidir-remote-be.ini";
    mkdir "multibe", 0777 unless -d "multibe";
    chdir "multibe";
}
elsif ($transport_type eq 'multire') {
    $trans_config_file = "$bench_location/etc/transport-multi-rel.ini ";
    $sub_config_file = "$bench_location/tests/thru/bidir-remote-rel.ini";
    mkdir "multire", 0777 unless -d "multire";
    chdir "multire";
}
elsif ($transport_type eq 'tcp') {
   $trans_config_file = "$bench_location/etc/transport-tcp.ini";
   $sub_config_file = "$bench_location/tests/thru/bidir-remote-rel.ini";
    mkdir "tcp", 0777 unless -d "tcp";
    chdir "tcp";
}
else {
    print "Unknown transport. Skipping...\n";
    exit 0;
}

if (scalar @ARGV > 1) {
    die "Invalid test number.\n" unless (($ARGV[1] > 0) && ($ARGV[1] < 14));
    $starting_test_number = $ARGV[1];
    $ending_test_number = $starting_test_number + 1;

}


for ( ; $starting_test_number < $ending_test_number; $starting_test_number++) {
    $status = 0;
    $pub_config_file = $pub_rel_config_files[$starting_test_number - 1];
    if (($transport_type eq 'udp') || ($transport_type eq 'multibe')){
        $pub_config_file = $pub_be_config_files[$starting_test_number - 1];
    }


    $CS = new PerlDDS::Cross_Sync (1, PerlACE::random_port(), PerlACE::random_port()
                            , $pub_config_file, $sub_config_file, "../test_list.txt");
    if (!$CS) {
        print "Crossplatform test pre-reqs not met. Skipping...\n";
        exit 0;
    }

    if ($transport_type eq 'udp') {
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
        $DCPSREPO = PerlDDS::create_process
                  ("$bench_location/bin/run_test",
                   "-S -t 180 -h iiop://$repo_host:$port1");

        print $DCPSREPO->CommandLine(). "\n";
        $DCPSREPO->Spawn ();
        if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
            print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
            $DCPSREPO->Kill ();
            exit 1;
        }

        $Publisher = PerlDDS::create_process
                  ("$bench_location/bin/run_test",
                   "$common_args -s $pub_config_file -v -f bidir$starting_test_number.results");

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

        $ir = $DCPSREPO->TerminateWaitKill(30);
        ## Ignore the return value because the run_test does not cleanly
        ## shutdown the repo when run with a duration.
        #if ($ir != 0) {
        #    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
        #    $status = 1;
        #}

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
        print "test $starting_test_number PASSED.\n";
    } else {
        print STDERR "test $starting_test_number FAILED.\n";
        $total_status -= 1
    }
}

chdir $current_dir;
exit $total_status;

