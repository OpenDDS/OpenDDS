eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use File::Basename qw(dirname);

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my @all_kinds = ('classic', 'cpp11');
my @kinds = scalar(@ARGV) ? @ARGV : @all_kinds;
my $failed = 0;
for my $kind (@kinds) {
  my $path = "$kind/no_init_before_deserialize_$kind";
  my $test = new PerlDDS::TestFramework();
  $test->process('test', $path);
  $test->start_process('test');
  $failed |= ($test->finish(5) ? 1 : 0);
}

exit $failed;
