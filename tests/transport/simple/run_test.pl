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

#
# Test parameters.
#
my $testTime = 60;
my $n_samples = 0;
my $sample_size = 0; # only goes to pub
my $quiet = 0;
my $shared_memory = 0;

if ($ARGV[0] eq 'shmem') {
  $shared_memory = 1;
  shift;
}

if ($ARGV[0] eq 'n') {
  $n_samples = 400;
}
elsif ($ARGV[0] eq 'bp') {
  $n_samples = 400;
  $sample_size = 128;
  $quiet = 1;
}

#
# Publisher parameters.
#
my $publisherId = 1;
my $publisherHost = "localhost";
my $publisherPort = PerlACE::random_port();

#
# Subscriber parameters.
#
my $subscriberId = 2;
my $subscriberHost = "localhost";
my $subscriberPort = PerlACE::random_port();
my $subreadyfile = "subready.txt";
unlink $subreadyfile, 'sub-pid.txt', 'pub-pid.txt';

#
# Subscriber command and arguments.
#
my $subscriberCmd  = "simple_subscriber";
my $subscriberArgs = "-p $publisherId:$publisherHost:$publisherPort "
                   . "-s $subscriberId:$subscriberHost:$subscriberPort";

#
# Publisher command and arguments.
#
my $publisherCmd  = "simple_publisher";
my $publisherArgs = "-p $publisherId:$publisherHost:$publisherPort "
                  . "-s $subscriberId:$subscriberHost:$subscriberPort";

if ($shared_memory) {
  $subscriberArgs .= ' -m';
  $publisherArgs  .= ' -m';
}

my $debug = '-DCPSDebugLevel 10 -DCPSTransportDebugLevel 10';
#$subscriberArgs .= " $debug";
#$publisherArgs .= " $debug";

if ($n_samples) {
  $subscriberArgs .= " -n $n_samples";
  $publisherArgs .= " -n $n_samples";
}

if ($sample_size) {
  $publisherArgs .= " -c $sample_size";
}

if ($quiet) {
  $subscriberArgs .= " -q";
  $publisherArgs .= " -q";
}

#
# Create the test objects.
#
my $subscriber = PerlDDS::create_process($subscriberCmd, $subscriberArgs);
my $publisher  = PerlDDS::create_process($publisherCmd, $publisherArgs);


#
# Fire up the subscriber first.
#
$subscriber->Spawn();
if (PerlACE::waitforfile_timed($subreadyfile, 30) == -1) {
    print STDERR "ERROR: waiting for subscriber file\n";
    $subscriber->Kill();
    exit 1;
}

$publisher->Spawn();

#
# Wait for the test to finish, or kill the processes.
#
die "*** ERROR: Subscriber timed out - $!" if $subscriber->WaitKill($testTime);
die "*** ERROR: Publisher timed out - $!"  if $publisher->WaitKill($testTime);

unlink $subreadyfile, 'sub-pid.txt', 'pub-pid.txt';

exit 0;
