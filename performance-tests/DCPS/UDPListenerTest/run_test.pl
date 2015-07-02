eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

PerlDDS::add_lib_path('../TypeNoKeyBounded');

sub usage {
    my $msg = shift;
    if ($msg ne "") {
        print STDERR "ERROR: $msg\n";
    }
    print STDERR "run_test.pl [-b] [-p n] [-s n]\n" .
        "    -b   enable Built In Topics (BIT)\n" .
        "    -p n start n publisher processes\n" .
        "    -s n start n subscriber processes\n";
    exit 1;
}

# single reader with single instances test
my $num_messages=500;
my $data_size=13;
my $num_writers=2;
my $num_readers=3;
my $num_msgs_btwn_rec=20;
my $pub_writer_id=0;
my $conf_file="conf.ini";

# default bit to off
my $repo_bit_conf = "-NOBITS ";
my $app_bit_conf = "-DCPSBit 0 ";

my $arg_ind = 0;
while ($arg_ind <= $#ARGV) {
    if ($ARGV[$arg_ind] eq '-b') {
        $repo_bit_conf = "";
        $app_bit_conf = "";
    }
    elsif ($ARGV[$arg_ind] eq '-p') {
        $arg_ind++;
        if ($arg_ind > $#ARGV) {
            usage("Missing param for -p");
        }
        $num_writers = $ARGV[$arg_ind];
    }
    elsif ($ARGV[$arg_ind] eq '-s') {
        $arg_ind++;
        if ($arg_ind > $#ARGV) {
            usage("Missing param for -s");
        }
        $num_readers = $ARGV[$arg_ind];
    }
    else {
        usage("Invalid parameter $ARGV[$arg_ind]");
    }
    $arg_ind++;
}



# need $num_msgs_btwn_rec unread samples plus 20 for good measure
# (possibly allocated by not yet queue by the transport because of greedy read).
my $num_samples=$num_msgs_btwn_rec + 20;

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                        "$repo_bit_conf -o $dcpsrepo_ior ");

print $DCPSREPO->CommandLine(), "\n";

my $sub_parameters = "$app_bit_conf -DCPSConfigFile $conf_file"
#              . " -DCPSDebugLevel 6"
              . " -p $num_writers"
              . " -i $num_msgs_btwn_rec"
              . " -n $num_messages -d $data_size"
              . " -msi $num_samples -mxs $num_samples";
#use -msi $num_messages to avoid rejected samples
#use -mxs $num_messages to avoid using the heap
#   (could be less than $num_messages but I am not sure of the limit).

my $i;
my @subs;
for ($i = 0; $i < $num_readers; $i++) {
    $subs[$i] = PerlDDS::create_process ("subscriber", $sub_parameters);
    print $subs[$i]->CommandLine(), "\n";
}

#NOTE: above 1000 queue samples does not give any better performance.
my $pub_parameters = "$app_bit_conf -DCPSConfigFile $conf_file"
#              . " -DCPSDebugLevel 6"
              . " -p 1"
              . " -r $num_readers"
              . " -n $num_messages -d $data_size"
              . " -msi 1000 -mxs 1000";

my @pubs;
for ($i = 0; $i < $num_writers; $i++) {
    $pubs[$i] = PerlDDS::create_process ("publisher",
                                         $pub_parameters . " -i $pub_writer_id");
    print $pubs[$i]->CommandLine(), "\n";
    $pub_writer_id++;
}


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

for ($i = 0; $i < $num_readers; $i++) {
    $subs[$i]->Spawn ();
}

for ($i = 0; $i < $num_writers; $i++) {
    $pubs[$i]->Spawn ();
}

my $wait_to_kill = 200;
for ($i = 0; $i < $num_writers; $i++) {
    my $PubResult = $pubs[$i]->WaitKill ($wait_to_kill);
    if ($PubResult != 0) {
        print STDERR "ERROR: publisher $i returned $PubResult \n";
        $status = 1;
        $wait_to_kill = 0;
    }
}

for ($i = 0; $i < $num_readers; $i++) {
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
