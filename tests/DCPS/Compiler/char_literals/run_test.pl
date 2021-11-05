eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use File::Basename qw(dirname);

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $this_dir = dirname(__FILE__);
opendir(DIR, $this_dir) || die $!;
my @files = readdir(DIR);
closedir(DIR) || die $!;
my @all_kinds;
for my $file (@files) {
  next if $file eq '.' || $file eq '..';
  push(@all_kinds, $file) if (-d "$this_dir/$file");
}

my @kinds = scalar(@ARGV) ? @ARGV : @all_kinds;
my $failed = 0;
for my $kind (@kinds) {
  my $path = "$kind/char-literals-$kind";

  my $test = new PerlDDS::TestFramework();
  $test->process('test', $path);
  $test->start_process('test');
  $failed |= ($test->finish(5) ? 1 : 0);
}

exit $failed;
