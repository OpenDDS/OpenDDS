eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

if ($ARGV[0]) {
  my $test = new PerlDDS::TestFramework();
  $test->process('test', $ARGV[0]);
  $test->start_process('test');
  exit $test->finish(60);
}

print STDERR "ERROR: provide the test name as a command line argument\n";
exit 1;
