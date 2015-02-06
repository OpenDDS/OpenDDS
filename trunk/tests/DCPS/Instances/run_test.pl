eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../../Utils/');

my $status=0;


# process control parameters
my $keyed_data=1;
my $multiple_instance=0;
my $num_instances=1;
my $num_writers=1;
my $debug_level=0;

# const process control parameters
my $num_threads_to_write=5;
my $num_writes_per_thread=2;
my $max_samples_per_instance= 12345678;
my $history_depth=100;
my $write_delay_msec=0;
my $receive_delay_msec=0;
my $publisher_running_sec=60;
my $subscriber_running_sec=30;
my $repo_bit_conf = "-NOBITS";
my $app_bit_conf = "-DCPSBit 0";
my $cfgfile = '';

for my $arg (@ARGV) {
    if ($arg eq 'keyed') {
        $keyed_data=1;
    }
    elsif ($arg eq 'nokey') {
        $keyed_data=0;
    }
    elsif ($arg eq 'single_instance') {
        $multiple_instance=0;
    }
    elsif ($arg eq 'multiple_instance') {
        $multiple_instance=1;
    }
    elsif ($arg eq 'single_datawriter') {
        $num_writers=1;
        $num_instances=$num_threads_to_write;
    }
    elsif ($arg eq 'multiple_datawriter') {
        $num_writers=4;
        $num_instances=$num_threads_to_write * $num_writers;
    }
    elsif ($arg eq 'debug') {
        $debug_level=4;
    }
    elsif ($arg ne '') {
        print STDERR "ERROR: invalid test case\n";
        exit 1;
    }
}

my $num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

my $dcpsrepo_ior="dcps.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO=PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                      "$repo_bit_conf -o $dcpsrepo_ior");
print $DCPSREPO->CommandLine(), "\n";

my $publisher=PerlDDS::create_process ("publisher",
                                       "$app_bit_conf $cfgfile"
                                       . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                       . " -DCPSDebugLevel $debug_level"
                                       . " -keyed_data $keyed_data"
                                       . " -num_writers $num_writers"
                                       . " -history_depth $history_depth"
                                       . " -num_threads_to_write $num_threads_to_write"
                                       . " -multiple_instances $multiple_instance"
                                       . " -num_writes_per_thread $num_writes_per_thread "
                                       . " -max_samples_per_instance $max_samples_per_instance"
                                       . " -write_delay_msec $write_delay_msec"
);

print $publisher->CommandLine(), "\n";

my $subscriber=PerlDDS::create_process ("subscriber",
                                        "$app_bit_conf $cfgfile"
                                        . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                        . " -DCPSDebugLevel $debug_level"
                                        . " -keyed_data $keyed_data"
                                        . " -num_writes $num_writes"
                                        . " -receive_delay_msec $receive_delay_msec");

print $subscriber->CommandLine(), "\n";

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$subscriber->Spawn ();
$publisher->Spawn ();

my $result=$publisher->WaitKill ($publisher_running_sec);

if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status=1;
}

$result=$subscriber->WaitKill($subscriber_running_sec);

if ($result != 0) {
    print STDERR "ERROR: $subscriber returned $result  \n";
    $status=1;
}

my $ir=$DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status=1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
