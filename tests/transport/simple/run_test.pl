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
my $testTime = 60;
my $n_samples = 0;

if ($ARGV[0] eq 'n') {
  $n_samples = 400;
}

#
# Publisher parameters.
#
my $publisherId = 1;
my $publisherHost = "localhost";
my $publisherPort = 10001 + PerlACE::uniqueid();

#
# Subscriber parameters.
#
my $subscriberId = 2;
my $subscriberHost = "localhost";
my $subscriberPort = 10002 + PerlACE::uniqueid();
my $subreadyfile = "subready.txt";
unlink $subreadyfile;

#
# Subscriber command and arguments.
#
my $subscriberCmd  = "./simple_subscriber";
my $subscriberArgs = "-p $publisherId:$publisherHost:$publisherPort "
                   . "-s $subscriberId:$subscriberHost:$subscriberPort";

#
# Publisher command and arguments.
#
my $publisherCmd  = "./simple_publisher";
my $publisherArgs = "-p $publisherId:$publisherHost:$publisherPort "
                  . "-s $subscriberId:$subscriberHost:$subscriberPort";

my $debug = '-DCPSDebugLevel 10 -DCPSTransportDebugLevel 10';
#$subscriberArgs .= " $debug";
#$publisherArgs .= " $debug";

if ($n_samples) {
  $subscriberArgs .= " -n $n_samples";
  $publisherArgs .= " -n $n_samples";
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
die "*** ERROR: Publisher timed out - $!"  if $publisher->WaitKill(5);

unlink $subreadyfile;

exit 0;
