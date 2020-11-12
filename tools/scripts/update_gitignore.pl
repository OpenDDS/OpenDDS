#!/usr/bin/env perl

use File::Basename;
use strict;

my %dirs;

open GIT, "git status --porcelain |";
while (<GIT>) {
  chomp;
  next unless /^\?\?\s+(\S+)/;
  my @fp = fileparse $1;
  if (scalar @ARGV) {
    $fp[0] =~ s/$ARGV[0]/*/g;
  }
  push(@{$dirs{$fp[1]}}, '/' . $fp[0]);
}
close GIT;

$SIG{PIPE} = 'IGNORE';

for my $dir (keys %dirs) {
  print "$dir: ", join(' ', @{$dirs{$dir}}), "\n";
  my $path = "$dir/.gitignore";
  my @existing;
  if (-r $path) {
    open GIIN, $path or die "can't open existing gitignore at $path ($!)";
    @existing = <GIIN>;
    close GIIN;
    chomp @existing;
  }
  open GIOUT, ">$path" or die "can't write $path ($!)";
  my @files = grep /\S/, @existing;
  push(@files, @{$dirs{$dir}});
  for my $file (sort {lc($a) cmp lc($b)} @files) {
    print GIOUT "$file\n";
  }
  close GIOUT;
}
print "DONE\n";
