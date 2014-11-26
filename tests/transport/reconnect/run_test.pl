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

#
# Test parameters.
#
my $testTime = 40;
my $n_samples = 0;
my $sample_size = 0; # only goes to pub
my $quiet = 0;
my $shared_memory = 0;
my $socat_kill_delay = 0;
my $num_socat_kills = 0;

if ($ARGV[0] eq 'shmem') {
  $shared_memory = 1;
  shift;
}

if ($ARGV[0] eq 'n') {
  $n_samples = 25;
  $socat_kill_delay = 5;
  $num_socat_kills = 3;
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

#
# socat parameters
#
my $socatPubSideHost  = "localhost";
my $socatPubSidePort = PerlACE::random_port();
my $socatSubSideHost = "localhost";
my $socatSubSidePort = PerlACE::random_port();

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
                  . "-s $subscriberId:$socatPubSideHost:$socatPubSidePort";

#
# socat command and arguments.
#
my $socatCmd = "/usr/bin/socat";
my $socatArgs = " -d -d TCP-LISTEN:$socatPubSidePort,reuseaddr " .
                "TCP:$subscriberHost:$subscriberPort";

if ($shared_memory) {
  $subscriberArgs .= ' -m';
  $publisherArgs  .= ' -m';
}

my $debug = '-DCPSDebugLevel 1 -ORBVerboseLogging 1 -DCPSTransportDebugLevel 1';
$subscriberArgs .= " $debug ";
$publisherArgs .= " $debug ";
#$subscriberArgs .= " -ORBLogFile sub.log ";
#$publisherArgs .= " -ORBLogFile pub.log ";

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
my $socat = PerlDDS::create_process($socatCmd, $socatArgs);
my $subscriber = PerlDDS::create_process($subscriberCmd, $subscriberArgs);
my $publisher  = PerlDDS::create_process($publisherCmd, $publisherArgs);


#
# Fire up the subscriber first.
#
print $subscriber->CommandLine() . "\n";
$subscriber->Spawn();
if (PerlACE::waitforfile_timed($subreadyfile, 30) == -1) {
    print STDERR "ERROR: waiting for subscriber file\n";
    $subscriber->Kill();
    exit 1;
}

#
# Start socat next
#
print $socat->CommandLine() . "\n";
$socat->Spawn();

print $publisher->CommandLine() . "\n";
$publisher->Spawn();

while ($num_socat_kills > 0) {
  print "Sleeping\n";
  sleep($socat_kill_delay);
  $socat->Kill();
  $socat->Spawn();
  $num_socat_kills--;
}

#
# Wait for the test to finish, or kill the processes.
#
print "Waiting for subscriber...\n";
die "*** ERROR: Subscriber timed out - $!" if $subscriber->WaitKill($testTime);
print "Waiting for publisher...\n";
die "*** ERROR: Publisher timed out - $!"  if $publisher->WaitKill($testTime);

print "Killing socat...\n";
$socat->Kill();

unlink $subreadyfile, 'sub-pid.txt', 'pub-pid.txt';

exit 0;
