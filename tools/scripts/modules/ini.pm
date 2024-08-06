#!/usr/bin/env perl

package ini;

use strict;
use warnings;

use Getopt::Long qw/GetOptions/;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
  read_ini_file
  get_ini_value
);

use FindBin;
use lib "$FindBin::RealBin";
use misc_utils qw/trace/;

sub print_usage {
  my $fh = shift() // *STDERR;

  print $fh ("Usage: ini.pm INI_FILE [--join JOINER] [--dump] [KEY...]\n");
}

sub print_help {
  print_usage(*STDOUT);
}

sub invalid_args {
  print STDERR ("ERROR: Invalid arguments, ", shift(), "\n");
  print_usage(*STDOUT);
  exit(1);
}

sub read_ini_file {
  my $path = shift();

  my @section_names;
  my %sections;
  my $section;
  open(my $fh, $path) or trace("Couldn't open \"$path\": $!");
  while (my $line = <$fh>) {
    $line =~ s/\s$//;
    if ($line =~ /\[(\w+)\]/) {
      push(@section_names, $1);
      $section = {};
      $sections{$1} = $section;
    }
    elsif ($section && $line =~ /([^=#]+)=(.*)/) {
      $section->{$1} = $2;
    }
  }

  return \@section_names, \%sections;
}

sub get_ini_value {
  my $section_names = shift();
  my $sections = shift();
  my $key = shift();
  my $dump = shift();

  my @results;
  for my $section_name (@{$section_names}) {
    my $section = $sections->{$section_name};
    for my $key_name (keys(%{$section})) {
      my $full_key = "$section_name/$key_name";
      if ($full_key =~ /$key/) {
        my $result = $section->{$key_name};
        if ($dump) {
          $result = "$full_key => $result";
        }
        push(@results, $result);
      }
    }
  }

  return @results;
}

sub main {
  my $help = 0;
  my $join = "\n";
  my $dump = 0;
  if (!GetOptions(
    'h|help' => \$help,
    'join=s' => \$join,
    'dump' => \$dump,
  )) {
    print_usage();
    exit(1);
  }
  if ($help) {
    print_help();
    exit(0);
  }

  my $ini_file = shift(@ARGV) or invalid_args("must pass INI_FILE");

  my ($section_names, $sections) = read_ini_file($ini_file);

  my @keys = @ARGV ? @ARGV : ".*/.*";
  my @results;
  for my $key (@keys) {
    push(@results, get_ini_value($section_names, $sections, $key, $dump || !@ARGV));
  }
  print(join($join, @results), "\n");
}

main() if not caller();

1;
