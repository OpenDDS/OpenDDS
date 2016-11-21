#!/usr/bin/perl
use strict;
use File::Copy;

my $pr = shift;
my $destination = shift;
my $rsync_extra = shift;

if (!-r 'post') {
  print "No 'post' file, exiting\n";
  exit 0;
}

open IN, 'latest.txt' or die "No 'latest.txt' file";
my $latest = <IN>;
close IN;

$latest =~ /^\S+/;
my $base = $&;
copy($base . '_Totals.html', 'index.html');

my $opts = '-vz -e ssh';
$opts .= " $rsync_extra" if defined $rsync_extra;

my $srcs = "index.html Tests_JUnit.xml $base*";
my $cmd = "rsync $opts $srcs $destination/OpenDDS/PR$pr/";
print "Running: $cmd\n";
my $ret = system($cmd);

unlink 'index.html';
if ($ret == 0) {
  unlink 'post';
}

exit $ret >> 8;
