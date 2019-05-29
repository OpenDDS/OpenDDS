eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env qw(ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;
use strict;

my $annotest_idl = new PerlACE::Process("RapidJsonTest", "");
print $annotest_idl->CommandLine () . "\n";
$annotest_idl->Spawn ();
my $annotest_idl_result = $annotest_idl->WaitKill (10);
if ($annotest_idl_result > 0) {
  print STDERR "ERROR: annotest_idl returned $annotest_idl_result\n";
}

my $status = 1 if $annotest_idl_result;

if ($status) {
  print STDERR "test FAILED\n";
}
else {
  print "test PASSED\n";
}

exit $status
