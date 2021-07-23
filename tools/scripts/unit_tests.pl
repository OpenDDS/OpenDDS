#!/usr/bin/env perl

use Cwd;
use strict;

my $pattern = "unit";

for my $arg (@ARGV) {
  if ($arg eq "stress") {
    $pattern = "stress";
  }
}

my @lists = ('tests/dcps_tests.lst');
push @lists, 'tests/security/security_tests.lst' if has_security();

my @lines;
for my $list (@lists) {
  open my $f, $ENV{'DDS_ROOT'} . '/' . $list or die "Failed to read $list";
  while (<$f>) {
    next if /^\s*#/;
    if (/$pattern/i) {
      push @lines, $_;
    }
  }
  close $f;
}

print "$0 running " . scalar @lines . " $pattern test programs\n";
my $start_time = time();
open my $f, "|$^X $ENV{'DDS_ROOT'}/tests/auto_run_tests.pl -x -l -"
    or die "Failed to launch auto_run_tests";
print $f @lines;
my $status = close($f) ? 0 : $?;
my $time = time() - $start_time;
print "$0 completed in ${time}s\n";
exit $status >> 8;

sub has_security {
  my $libdir = $ENV{'DDS_ROOT'} . '/lib';
  glob "$libdir/libOpenDDS_Security.*" || glob "$libdir/OpenDDS_Security*";
}
