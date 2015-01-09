#!/usr/bin/perl

use File::Basename;
use strict;

my %dirs;

open SVN, "svn st|";
while (<SVN>) {
  chomp;
  next unless /^\?\s+(\S+)/;
  my @fp = fileparse $1;
  if (scalar @ARGV) {
    $fp[0] =~ s/$ARGV[0]/*/g;
  }
  push(@{$dirs{$fp[1]}}, $fp[0]);
}
close SVN;

$SIG{PIPE} = 'IGNORE';

for my $dir (keys %dirs) {
  print "$dir: ", join(' ', @{$dirs{$dir}}), "\n";
  open SVN, "svn pg svn:ignore $dir|" or die "can't spawn svn pg";
  my @existing = <SVN>;
  close SVN;
  chomp @existing;
  open SVN, "|svn ps svn:ignore -F - $dir" or die "can't spawn svn ps";
  my @files = grep /\S/, @existing;
  push(@files, @{$dirs{$dir}});
  for my $file (sort {lc($a) cmp lc($b)} @files) {
    print SVN "$file\n";
  }
  close SVN;
}
print "DONE\n";
