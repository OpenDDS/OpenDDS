#!/usr/bin/perl

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
  my @existing;
  if (-r "$dir/.gitignore") {
    open GIIN, "$dir/.gitignore" or die "can't open existing .gitignore";
    @existing = <GIIN>;
    close GIIN;
    chomp @existing;
  }
  open GIOUT, ">$dir/.gitignore" or die "can't write to .gitignore";
  my @files = grep /\S/, @existing;
  push(@files, @{$dirs{$dir}});
  for my $file (sort {lc($a) cmp lc($b)} @files) {
    print GIOUT "$file\n";
  }
  close SVN;
}
print "DONE\n";
