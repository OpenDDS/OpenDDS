eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

my $status = 0;
my $failed = 0;

# Name the pieces.

# Configure the subsystems.

my $router_parameters = "-FileName test.in ";
my $Router = new PerlACE::Process ("router", $router_parameters);
print $Router->CommandLine(), "\n";

# Fire up the test process.

$Router->Spawn ();

# Wait up to 5 minutes for test to complete.

$status = $Router->WaitKill (300);
if ($status != 0) {
    print STDERR "ERROR: router returned $status\n";
}
$failed += $status;

# Report results.

if ($failed == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;

