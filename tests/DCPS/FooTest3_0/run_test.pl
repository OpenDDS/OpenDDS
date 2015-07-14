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

# Set the library path for the client to be able to load
# the FooType* library.
PerlDDS::add_lib_path('../FooType3');

my $status = 0;

# Configuration for default test - register test.
my $test_to_run = 0;
my $num_writes = 2;
my $n_chunks = 20; # Number of pre-allocated chunks for Dynamic_Cached_Allocator
my $shutdown_pub = 1;
my $add_new_subscription = 0;
my $shutdown_delay_secs=10;
my $sub_ready_file = "sub_ready.txt";

if ($ARGV[0] eq 'unregister') {
    $test_to_run = 1;
    $num_writes = 4;  # 1 unregister, 3 writes.
}
elsif ($ARGV[0] eq 'dispose') {
    $test_to_run = 2;
    $num_writes = 11;  # 1 dispose and 10 writes.
}
elsif ($ARGV[0] eq 'resume') {
    $test_to_run = 3;
    $num_writes = 10;  # 10 writes.
}
elsif ($ARGV[0] eq 'listener') {
    $test_to_run = 4;
    # Same number of writes as register_test
}
elsif ($ARGV[0] eq 'allocator') {
    $test_to_run = 5;
    $n_chunks = 2;
    $num_writes = 4;  # 4 writes - 2 allocate from pool and 2 allocate from heap.
}
elsif ($ARGV[0] eq 'liveliness') {
    $test_to_run = 6;
    $num_writes = 3;  # 3 writes
}
elsif ($ARGV[0] eq '') { # register test
    # default register test: 1 register and 2 writes.
}
else {
    print STDERR "ERROR: invalid parameter $ARGV[0] \n";
    exit 1;
}

my $dcpsrepo_ior = "repo.ior";

my $history_depth=10;
my $repo_bit_conf = "-NOBITS";
my $app_bit_conf = "-DCPSBit 0";

unlink $dcpsrepo_ior;
unlink $sub_ready_file;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                        "-o $dcpsrepo_ior $repo_bit_conf");
print $DCPSREPO->CommandLine(), "\n";

my $publisher = PerlDDS::create_process ("FooTest3_publisher",
                                         "$app_bit_conf "
                                         . " -DCPSInfoRepo file://$dcpsrepo_ior -d $history_depth"
                                         . " -t $test_to_run -DCPSChunks $n_chunks"
                                         . " -f $sub_ready_file");

print $publisher->CommandLine(), "\n";

my $subscriber = PerlDDS::create_process ("FooTest3_subscriber",
                                          " -DCPSInfoRepo file://$dcpsrepo_ior "
                                          . "$app_bit_conf -n $num_writes "
                                          . "-x $shutdown_pub "
                                          . "-a $add_new_subscription -d $shutdown_delay_secs "
                                          . "-f $sub_ready_file");

print $subscriber->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$subscriber->Spawn ();
$publisher->Spawn ();

$status |= PerlDDS::wait_kill($subscriber, 60, "subscriber");

$status |= PerlDDS::wait_kill($publisher, 60, "publisher");
$status |= PerlDDS::terminate_wait_kill($DCPSREPO);

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

unlink $dcpsrepo_ior;
unlink $sub_ready_file;

exit $status;
