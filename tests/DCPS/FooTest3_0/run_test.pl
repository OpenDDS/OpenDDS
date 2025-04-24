eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Path;
use strict;

# Set the library path for the client to be able to load
# the FooType* library.
PerlDDS::add_lib_path('../FooType3');

my $status = 0;

# Configuration for default test - register test.
my $test_to_run = 0;
my $num_writes = 2;
my $num_disposed = 0;
my $n_chunks = 20; # Number of pre-allocated chunks for Dynamic_Cached_Allocator
my $shutdown_pub = 1;
my $add_new_subscription = 0;
my $shutdown_delay_secs=10;
my $use_qc = 0;

if ($ARGV[0] eq 'unregister') {
    $test_to_run = 1;
    $num_writes = 4;  # 1 unregister, 3 writes.
}
elsif ($ARGV[0] eq 'unregister_nil') {
    $test_to_run = 2;
    $num_writes = 4;  # 1 register, 2 writes, 2 unregister
}
elsif ($ARGV[0] eq 'dispose') {
    $test_to_run = 3;
    $num_disposed = 1;
    $num_writes = 11;  # 1 dispose and 10 writes.
}
elsif ($ARGV[0] eq 'dispose_qc') {
    $test_to_run = 3;
    $num_disposed = 1;
    $use_qc = 1;
    $num_writes = 11;  # 1 dispose and 10 writes.
}
elsif ($ARGV[0] eq 'resume') {
    $test_to_run = 4;
    $num_writes = 10;  # 10 writes.
}
elsif ($ARGV[0] eq 'listener') {
    $test_to_run = 5;
    # Same number of writes as register_test
}
elsif ($ARGV[0] eq 'allocator') {
    $test_to_run = 6;
    $n_chunks = 2;
    $num_writes = 4;  # 4 writes - 2 allocate from pool and 2 allocate from heap.
}
elsif ($ARGV[0] eq 'liveliness') {
    $test_to_run = 7;
    $num_writes = 3;  # 3 writes
}
elsif ($ARGV[0] eq '') { # register test
    # default register test: 1 register and 2 writes.
}
else {
    print STDERR "ERROR: invalid parameter $ARGV[0]\n";
    exit 1;
}

my $history_depth=10;

my $publisher = PerlDDS::create_process ("FooTest3_publisher",
                                         "-d $history_depth"
                                         . " -t $test_to_run -DCPSChunks $n_chunks");

print $publisher->CommandLine(), "\n";

my $subscriber = PerlDDS::create_process ("FooTest3_subscriber",
                                          "-n $num_writes "
                                          . "-x $shutdown_pub "
                                          . "-a $add_new_subscription -d $shutdown_delay_secs -i $num_disposed -q $use_qc");

print $subscriber->CommandLine(), "\n";

rmtree('./DCS');

$subscriber->Spawn ();
$publisher->Spawn ();

$status |= PerlDDS::wait_kill($subscriber, 60, "subscriber");

$status |= PerlDDS::wait_kill($publisher, 60, "publisher");

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
