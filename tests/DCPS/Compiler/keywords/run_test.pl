eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $kind = $ARGV[0];
my $path = "$kind/keywords-$kind";

my $test = new PerlDDS::TestFramework();
$test->process('test', $path);
$test->start_process('test');
exit $test->finish(5) ? 1 : 0;
