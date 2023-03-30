eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

use Getopt::Long;

my $test = new PerlDDS::TestFramework();

my $dynamic = 0;
GetOptions(
  'dynamic' => \$dynamic,
) or die("Invalid options");

my @args;
my @exclude;
if ($dynamic) {
  # TODO: Remove this when dynamic XCDR1 is supported
  push(@exclude, '*Xcdr1*');
  push(@args, '--dynamic');
}
if (scalar(@exclude)) {
  push(@args, '--gtest_filter=-' . join(':', @exclude));
}

print(join(' ', @args), "\n");
$test->process('test', 'xcdr', join(' ', @args));
$test->start_process('test');
exit $test->finish(30) ? 1 : 0;
