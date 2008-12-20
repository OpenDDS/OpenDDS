eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

########################################################################
#
# Process the command line.
#

my $status = 0;
my $failed = 0;

# Change how test is configured according to which test we are.

$publisher_args;
$subscriber1_args;
$subscriber2_args;

my $PUBLISHER;
my $SUBSCRIBER1;
my $SUBSCRIBER2;

if (PerlACE::is_vxworks_test()) {
  $PUBLISHER   = new PerlACE::ProcessVX ("publisher", $publisher_args);
  $SUBSCRIBER1 = new PerlACE::ProcessVX ("subscriber", $subscriber1_args);
  $SUBSCRIBER2 = new PerlACE::ProcessVX ("subscriber", $subscriber2_args);

} else {
  $PUBLISHER   = new PerlACE::Process ("publisher", $publisher_args);
  $SUBSCRIBER1 = new PerlACE::Process ("subscriber", $subscriber1_args);
  $SUBSCRIBER2 = new PerlACE::Process ("subscriber", $subscriber2_args);

}

unlink $repo_ior;

# Fire up the repository.
system "ospl start";

# Fire up the publisher
print $PUBLISHER->CommandLine(), "\n";
$PUBLISHER->Spawn ();

# Fire up the subscribers
print $SUBSCRIBER1->CommandLine(), "\n";
$SUBSCRIBER1->Spawn ();

print $SUBSCRIBER2->CommandLine(), "\n";
$SUBSCRIBER2->Spawn ();

# Wait up to 5 minutes for test to complete.

$status = $SUBSCRIBER1->WaitKill (300);
if ($status != 0) {
    print STDERR "ERROR: Subscriber 1 returned $status\n";
}
$failed += $status;

$status = $SUBSCRIBER2->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: Subscriber 2 returned $status\n";
}
$failed += $status;

# And it can, in the worst case, take up to half a minute to shut down the rest.

$status = $PUBLISHER->WaitKill (15);
if ($status != 0) {
    print STDERR "ERROR: Publisher returned $status\n";
}
$failed += $status;

system "ospl stop";

# Report results.

if ($failed == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;


