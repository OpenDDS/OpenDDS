eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;

=head1 NAME

reduce-latency-data.pl - reduce test results into plottable data


=head1 SYNOPSIS

  reduce-latency-data.pl <infile>

=head1 DESCRIPTION

This script processes the input file and prints converted data to
standard output.

The input file is expected to be in the format produced by the OpenDDS
performance test bench latency data summary.

The output consists of data suitable for plotting using GNUPlot.  There
are 5 indexed sections with the following data in the columns of each
section:

=over 8

=item B<Index 0>

Latency and jitter data.

This index does not include a sample number.  This can be derived by
using $0 for the x axis in the GNUPlot plot command.

=over 8

=item B<Column 1>

Individual data points for 1/HOPS full path latency from the input file.
HOPS is set internally to a value of 2 corresponding to the
pre-configured OpenDDS-Bench latency tests.

=item B<Column 2>

Individual data points for jitter between successive latency data points.

=back

=item B<Index 1>

Latency histogram data.  This index has binned data derived from the
index 0 / column 1 data points.  There are currently 25 bins into which
the data is placed.

=over 8

=item B<Column 1>

The center of each bin.

=item B<Column 2>

The frequency (number of samples) in the bin.

=back

=item B<Index 2>

Jitter histogram data.  This index has binned data derived from the
index 0 / column 2 data points.  There are currently 25 bins into which
the data is placed.

=over 8

=item B<Column 1>

The center of each bin.

=item B<Column 2>

The frequency (number of samples) in the bin.

=back

=item B<Index 3>

Latency quantile data.  This index has sorted latency data derived from
the index 0 / column 1 data points.

=over 8

=item B<Column 1>

Latency data from Index 0, sorted.

=back

=item B<Index 4>

Jitter quantile data.  This index has sorted jitter data derived from
the index 0 / column 2 data points.

=over 8

=item B<Column 1>

Jitter data from Index 0, sorted.

=back

=back

Each index section has a header comment.  The histogram sections (Index 1
and Index 2) have statistical summary data included as well.  This
statistical summary data is usable by the 'extract.pl' script to provide
plottable summary data.

If the data produced by this script is to be used by the 'extract.pl'
script, then the output file needs to be named such that it consists of
three '-' separated fields followed by the extension ".gpd" (representing
GNUPlot data file).  The fields represent the test type, the transport
type, and the message size of the test data.

=head1 EXAMPLE

  reduce-latency-data.pl tcp/latency-1000.data > data/latency-tcp-1000.gpd

=cut

# LOWER and UPPER are the upper and lower percentiles bounding the
# histogram data.
# BINS is the number of bins that the histogram data is partitioned into.
# HOPS is the number of hops in the collected data.  This is normalized out.
use constant LOWER => 0.05;
use constant UPPER => 0.95;
use constant BINS  => 25;
use constant HOPS  => 1;

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
#$skip = 0 if /^round_trip_latency/;
#$skip = 1 if /^ --- last hop/;
#next if $skip;

next if /^round_trip_latency/;

# Comment out non-data lines.
chomp;
my @discard = split;
do { print "# $_\n"; next } if 1 != scalar @discard;

# Value desired is actually single hop.
my $value = $_ / HOPS;

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
print "$value";
print " $jitter" if defined $jitter;
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
  print "#      Median: " . sprintf( "%5.2e", &median($data->{latency})) . "\n";
  print "#         MAD: " . sprintf( "%5.2e", &MAD($data->{latency})) . "\n";
  print "#     Maximum: " . sprintf( "%5.2e", $data->{maxlatency}) . "\n";
  print "#     Minimum: " . sprintf( "%5.2e", $data->{minlatency}) . "\n";
  print "#    Pearson2: " . sprintf( "%5.2e", (3*&median($data->{latency}) - &mean($data->{latency})/3.0)) . "\n";
  print "#\n";
  &bin( $data->{latency}, LOWER, UPPER);

  print "\n\n";
  print "#\n";
  print "# Index 2 - Jitter distribution histogram data.\n";
  print "#\n";
  print "#        Mean: " . sprintf( "%5.2e", &mean($data->{jitter})) . "\n";
  print "#   Std. Dev.: " . sprintf( "%5.2e", &std_dev($data->{jitter})) . "\n";
  print "#      Median: " . sprintf( "%5.2e", &median($data->{jitter})) . "\n";
  print "#         MAD: " . sprintf( "%5.2e", &MAD($data->{jitter})) . "\n";
  print "#     Maximum: " . sprintf( "%5.2e", $data->{maxjitter}) . "\n";
  print "#     Minimum: " . sprintf( "%5.2e", $data->{minjitter}) . "\n";
  print "#\n";
  &bin( $data->{jitter}, 0, 100);

  # Quantile data goes at the end.
  # TODO: The sorts could be rolled up into the processing above to reduce the
  #       total number of data sorts done.

  print "\n\n";
  print "#\n";
  print "# Index 3 - Latency Quantile data.\n";
  print "#\n";
  map { print "$_\n" } sort { $a <=> $b; } @{$data->{latency}};

  print "\n\n";
  print "#\n";
  print "# Index 4 - Jitter Quantile data.\n";
  print "#\n";
  map { print "$_\n" } sort { $a <=> $b; } @{$data->{jitter}};
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

sub median {
  my $values  = shift;
  my @ordered = sort { $a <=> $b; } @$values;
  my $size    = scalar @$values;
  if( not ($size % 2)) {
    return $ordered[ $size / 2];
  } else {
    my $lower = int( $size / 2);
    return ($ordered[ $lower] + $ordered[ $lower + 1]) / 2;
  }
}

sub MAD {
  my $values   = shift;
  my $median   = median( $values);
  my @deviants = map abs( $median - $_), @$values;
  return median( \@deviants);
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

