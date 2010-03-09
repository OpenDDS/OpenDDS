eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;
use File::Basename;

=pod

$Id$

extract.pl - extract summary statistics from plot datafiles.

SYNOPSIS
  extract.pl <infile> ...

This script processes the input files and prints a summary from all files
in comma separated values (CSV) format to standard output.

The input file is expected to be in the format produced by the reduce.pl
data reduction script.  This file type has statistical summary data in
the Index 1 and Index 2 section header comments that are parsed by this
script and gathered from all input files.

This input file name is expected to be in a format that includes '-'
separated fields and a fixed extension of ".gpd".

  <transport>-<size>.gpd

The <transport> and <size> fields are used to populate two columns in the
output data.

This output consists of a single CSV file with a single record (line)
generated from each input file.  The output files include:

  Field  1: transport type (derived from input filename)
  Field  2: test message size (derived from input filename)
  Field  3: latency mean statistic
  Field  4: latency standard deviation statistic
  Field  5: latency maximum statistic
  Field  6: latency minimum statistic
  Field  7: jitter mean statistic
  Field  8: jitter standard deviation statistic
  Field  9: jitter maximum statistic
  Field 10: jitter minimum statistic

EXAMPLE

  extract.pl data/*.gpd > data/latency.csv

=cut

# Filename extension to elide.
use constant EXTENSION => ".gpd";

# current   - current filename being processed.
# transport - current transport being processed.
# size      - current message size being processed.
# section   - current section of file being processed.
# data      - data that has been processed.
our ($current, $transport, $size, $section, $data);

if( not defined $current or $ARGV ne "$current") {
  # Starting a new file.
  ($transport, $size) = split "-", basename( $ARGV, EXTENSION);
  $current = $ARGV;
  undef $section;
}

# Track which data section (of interest) of the file we are currently in.
SECTION: {
  /Index 1/ && do { $section = "latency"; last SECTION; };
  /Index 2/ && do { $section = "jitter";  last SECTION; };
}

# No need to examine data if we are not in a section of interest.
next if not defined $section;

# Store data as it is recognized.
my $key;
/^#\s+Mean: (\S+)/      && do { $key = $section . "mean"; };
/^#\s+Std. Dev.: (\S+)/ && do { $key = $section . "dev"; };
/^#\s+Maximum: (\S+)/   && do { $key = $section . "max"; };
/^#\s+Minimum: (\S+)/   && do { $key = $section . "min"; };
$data->{ $transport}->{ $size}->{ $key } = $1 if $key;

END {
  # Format output as CSV data.
  my $index = 0;
  foreach my $transport (sort keys %$data) {
    print "#\n";
    print "# Index " . $index++ . " data for $transport.\n";
    print "#\n";
    print "# transport, size, latency mean, latency std. dev,\n";
    print "#   latency max, latency min, jitter mean, jitter dev,\n";
    print "#   jitter max, jitter min\n";
    print "#\n";
    foreach my $size (sort { $a <=> $b; } keys %{$data->{$transport}}) {
      print "$transport,";
      print "$size,";
      print "$data->{$transport}->{$size}->{latencymean},";
      print "$data->{$transport}->{$size}->{latencydev},";
      print "$data->{$transport}->{$size}->{latencymax},";
      print "$data->{$transport}->{$size}->{latencymin},";
      print "$data->{$transport}->{$size}->{jittermean},";
      print "$data->{$transport}->{$size}->{jitterdev},";
      print "$data->{$transport}->{$size}->{jittermax},";
      print "$data->{$transport}->{$size}->{jittermin}";
      print "\n";
    }
    print "\n\n";
  }
}

