#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;
use FindBin;
use Getopt::Long;

my $root = "$FindBin::RealBin/../..";
my $wildcard_replace = undef;
my @sort_existing = ();
GetOptions(
  'r|root=s' => \$root,
  'w|wildcard-replace=s' => \$wildcard_replace,
  's|sort-existing=s' => \@sort_existing,
) or die("Invalid options");

chdir($root) or die("Couldn't chdir to $root: $!");

my %dirs = map { $_ => [] } @sort_existing;

open GIT, "git status --porcelain |";
while (<GIT>) {
  chomp;
  next unless /^\?\?\s+(\S+)/;
  my @fp = fileparse $1;
  if (defined($wildcard_replace)) {
    $fp[0] =~ s/$wildcard_replace/*/g;
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
