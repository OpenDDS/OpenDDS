eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env qw(ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;
use strict;

my $name = "VreadVwriteTest";
my $test = new PerlACE::Process($name, "");
print $test->CommandLine () . "\n";
$test->Spawn ();
my $result = $test->WaitKill (10);
if ($result > 0) {
  print STDERR "ERROR: $name returned $result\n";
}

my $status = 1 if $result;

if ($status) {
  print STDERR "test FAILED\n";
}
else {
  print "test PASSED\n";
}

exit $status
