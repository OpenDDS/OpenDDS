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

PerlDDS::add_lib_path('../FooType3NoKey');

my $status = 0;

# The NO KEY type writer always has a single instance.
# We use the $multiple_instance=1 to let the test to
# give the different the key position value. This would
# verify the instance handle is only one handle with no key
# data type.
my $multiple_instance=1;

my $has_key = 0;
my $num_threads_to_write=5;
my $num_writes_per_thread=2;
my $num_writers=1;
#Make max_samples_per_instance large enough.
my $max_samples_per_instance= 12345678;
my $history_depth=100;
my $blocking_write=0;
my $repo_bit_conf = "-NOBITS";
my $app_bit_conf = "-DCPSBit 0";


# multiple datawriters test
if ($ARGV[0] eq 'mw') {
  $num_threads_to_write=2;
  $num_writers=2;
}
# multiple datawriters test with blocking write
elsif ($ARGV[0] eq 'mwb') {
  $num_writers=2;
  $blocking_write=1;
}
#tbd: add test for message dropped due to the depth limit.
elsif ($ARGV[0] eq '') {
  # default test
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

my $num_writes=$num_threads_to_write * $num_writes_per_thread * $num_writers;

my $dcpsrepo_ior = "dcps.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                        "$repo_bit_conf -o $dcpsrepo_ior");

my $publisher = PerlDDS::create_process ("FooTest3NoKey_publisher",
                                         "$app_bit_conf"
                                         . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                         . " -t $num_threads_to_write"
                                         . " -w $num_writers"
                                         . " -m $multiple_instance"
                                         . " -i $num_writes_per_thread"
                                         . " -n $max_samples_per_instance"
                                         . " -d $history_depth"
                                         . " -y $has_key"
                                         . " -b $blocking_write");

print $publisher->CommandLine(), "\n";

my $subscriber = PerlDDS::create_process ("FooTest3NoKey_subscriber",
                                          "$app_bit_conf"
                                          . " -DCPSInfoRepo file://$dcpsrepo_ior"
                                          . " -n $num_writes");

print $subscriber->CommandLine(), "\n";

$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$subscriber->Spawn ();
$publisher->Spawn ();

my $result = $publisher->WaitKill (60);

if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status = 1;
}


$result = $subscriber->WaitKill(60);

if ($result != 0) {
    print STDERR "ERROR: $subscriber returned $result  \n";
    $status = 1;
}


my $ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
