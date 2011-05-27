eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;

=pod

$Id$

reduce.pl - reduce test results into plottable data.

SYNOPSIS
  reduce.pl <infile>

This script processes the input file and prints converted data to
standard output.

The input file is expected to be in the format produced by the OpenDDS
performance test bench latency data summary.

The output consists of data suitable for plotting using GNUPlot.  There
are 4 indexed sections with the following data in the columns of each
section:

Index 0 -- latency and jitter data
           This index does not include a sample number.  This can be
           derived by using $0 for the x axis in the GNUPlot plot command.
  Column 1 - individual data points for 1/2 full path latency from the
             input file.
  Column 2 - individual data points for jitter between successive latency
             data points.

Index 1 -- latency histogram data
           This index has binned data derived from the index 0 / column 1
           data points.  There are currently 25 bins into which the data
           is placed.
  Column 1 - the center of each bin
  Column 2 - the frequency (number of samples) in the bin

Index 2 -- jitter histogram data
           This index has binned data derived from the index 0 / column 2
           data points.  There are currently 25 bins into which the data
           is placed.
  Column 1 - the center of each bin
  Column 2 - the frequency (number of samples) in the bin

Each index section has a header comment.  The histogram sections (Index 1
and Index 2) have statistical summary data included as well.  This
statistical summary data is usable by the 'extract.pl' script to provide
plottable summary data.

If the data produced by this script is to be used by the 'extract.pl'
script, then the output file needs to be named such that it consists of
two '-' separated fields followed by the extension ".gpd" (representing
GNUPlot data file).  The fields represent the transport type and the
message size of the test data.

EXAMPLE

  reduce.pl tcp/run/latency-1000.data > data/tcp-1000.gpd

=cut

# LOWER and UPPER are the upper and lower percentiles bounding the
# histogram data.
# BINS is the number of bins that the histogram data is partitioned into.
use constant LOWER => 0.05;
use constant UPPER => 0.95;
use constant BINS => 25;

# skip     - indicates whether to process a record or not.
# previous - is the previous record data.
# data     - holds the processed data values.
our ($skip, $previous, $data);

BEGIN {
  # Start input by skipping to the first Full Path section.
  $skip = 1;

  # Start output with a comment indicating what the current data index contains.
  print "#\n";
  print "# Index 0 - Latency and Jitter data.\n";
  print "#\n";
}

# Only process sections that are for the full path.
$skip = 0 if /^ --- full path/;
$skip = 1 if /^ --- last hop/;
next if $skip;

# Comment out non-data lines.
chomp;
my @discard = split;
do { print "# $_\n"; next } if 1 != scalar @discard;

# Value is actually single hop, or one half of the full path latency.
my $value = $_ / 2;

# Jitter is the difference in latency between adjacent measurements.
my $jitter;
$jitter = $value - $previous if $previous;

# Save the data and the max/min values of the data.
push @{$data->{latency}}, $value;
push @{$data->{jitter}},  $jitter if defined $jitter;
$data->{maxlatency} = $value  if not $data->{maxlatency} or $value  > $data->{maxlatency};
$data->{minlatency} = $value  if not $data->{minlatency} or $value  < $data->{minlatency};
$data->{maxjitter}  = $jitter if not $data->{maxjitter}  or $jitter > $data->{maxjitter};
$data->{minjitter}  = $jitter if not $data->{minjitter}  or $jitter < $data->{minjitter};

# Print the raw latency and jitter data.
print "$value ";
print $jitter if defined $jitter;
print "\n";

# Move to the next sample.
$previous = $value;

# Print histogram data once all measurements have been examined.
END {
  print "\n\n";
  print "#\n";
  print "# Index 1 - Latency distribution histogram data.\n";
  print "#\n";
  print "#        Mean: " . sprintf( "%5.2e", &mean($data->{latency})) . "\n";
  print "#   Std. Dev.: " . sprintf( "%5.2e", &std_dev($data->{latency})) . "\n";
  print "#     Maximum: " . sprintf( "%5.2e", $data->{maxlatency}) . "\n";
  print "#     Minimum: " . sprintf( "%5.2e", $data->{minlatency}) . "\n";
  print "#\n";
  &bin( $data->{latency}, LOWER, UPPER);

  print "\n\n";
  print "#\n";
  print "# Index 2 - Jitter distribution histogram data.\n";
  print "#\n";
  print "#        Mean: " . sprintf( "%5.2e", &mean($data->{jitter})) . "\n";
  print "#   Std. Dev.: " . sprintf( "%5.2e", &std_dev($data->{jitter})) . "\n";
  print "#     Maximum: " . sprintf( "%5.2e", $data->{maxjitter}) . "\n";
  print "#     Minimum: " . sprintf( "%5.2e", $data->{minjitter}) . "\n";
  print "#\n";
  &bin( $data->{jitter}, 0, 100);
}

# Bin data into histogram format.
sub bin {
  my( $values, $lower, $upper) = @_;
  my $size = scalar @$values;

  # Sort the values to extract the bounds.
  my @ordered = sort { $a <=> $b; } @$values;

  # Derive the lower bound.
  my $index = int($lower * $size) - 1;
  $index    = 0 if $index < 0;
  my $min   = $ordered[ $index];

  # Derive the upper bound.
  $index  = int($upper * $size) - 1;
  $index  = $size - 1 if $index > ($size - 1);
  my $max = $ordered[ $index];

  # Derive the bin size.
  my $span = ($max - $min) / BINS;

  print "# " . BINS . " bin histogram of ";
  print "$size points with bin width of ";
  print "$span from $min to $max.\n";

  # Traverse all the bins.
  for( my $current = $min; $current <= $max; $current += $span) {
    # Print the data for the current bin.
    printf "%9g", ($current - ($span/2.0));
    print " ";
    print scalar grep { (($current - $span) < $_) and ($_ <= $current) } @$values;
    print "\n";
  }
}

sub mean {
  my $values = shift;
  my $result;
  foreach my $value (@$values) { $result += $value }
  return $result / scalar @$values;
}

sub std_dev {
  my $values = shift;
  my $mean = mean($values);
  my $squared;
  @$squared = map { $_ * $_} @$values;
  return sqrt( mean($squared) - ($mean * $mean));
}

