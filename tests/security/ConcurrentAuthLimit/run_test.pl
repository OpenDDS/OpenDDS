eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();
$test->{dcps_security_debug} = 'auth_debug';

$test->process('ConcurrentAuthLimit', 'ConcurrentAuthLimit', "");
if ($test->flag('no_limit')) {
    $ENV{'no_limit'} = 'true';
}
$test->start_process('ConcurrentAuthLimit');
my $result = $test->finish(10);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
  exit 1;
}

exit 0;
