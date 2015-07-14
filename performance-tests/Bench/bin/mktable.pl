eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;

=head1 NAME

mktable.pl - create a TiddlyWiki or HTML table with latency test data


=head1 SYNOPSIS

  mktable.pl [ html ] <infile>

=head1 OPTIONS

=over 8

=item B<html>

Select HTML output format instead of the default TiddlyWiki tiddler
table format.  B<This is not currently supported>.

=back

=head1 DESCRIPTION

This script reads a data file with statistical information and creates
output suitable for inclusion within a tiddler in a TiddlyWiki document
to define a table with the data included.  If the optional 'html'
argument is supplied the output format will be as a static HTML table.

The input file is expected to be in the format produced by the extract.pl
data reduction script.  Each record (line) of the input file contains the
following fields:

=begin html

<table border>
  <tr><th>Field</th><th>Description</th></tr>
  <tr><td>1</td><td>transport type</td></tr>
  <tr><td>2</td><td>test message size</td></tr>
  <tr><td>3</td><td>latency mean statistic</td></tr>
  <tr><td>4</td><td>latency standard deviation statistic</td></tr>
  <tr><td>5</td><td>latency maximum statistic</td></tr>
  <tr><td>6</td><td>latency minimum statistic</td></tr>
  <tr><td>7</td><td>jitter mean statistic</td></tr>
  <tr><td>8</td><td>jitter standard deviation statistic</td></tr>
  <tr><td>9</td><td>jitter maximum statistic</td></tr>
  <tr><td>10</td><td>jitter minimum statistic</td></tr>
</table>

=end html

=for text
      Field 0: transport type
      Field 1: test message size
      Field 2: latency mean statistic
      Field 3: latency standard deviation statistic
      Field 4: latency maximum statistic
      Field 5: latency minimum statistic
      Field 6: jitter mean statistic
      Field 7: jitter standard deviation statistic
      Field 8: jitter maximum statistic
      Field 9: jitter minimum statistic

=for text

This script will only successfully create table data for input files that
contain the same number of message size data for each transport included.

The tiddler table output consists of TiddlyWiki table format definitions
to create a table containing the statistical information and links to
tiddlers that should contain the quad-plots representing the summarized
data.

The first two rows of the table include a row and column title for the
leftmost column identifying the transport along with the data columns -
one pair for each data size.

Subsequent rows include sets of 5 rows each representing summary data for
a specific transport type.  These rows contain the following columns:

=begin html

<table border>
  <tr><th>Row</th><th>Column(s)</th><th>Tiddler Format Contents</th><th>HTML Format Contents</th></tr>
  <tr><td>n</td><td>1</td><td>transport identification (name)</td><td>unsupported</td></tr>
  <tr><td>n</td><td>even</td><td>"Latency" label</td><td>unsupported</td></tr>
  <tr><td>n</td><td>odd, after first</td><td>"Mean&lt;br>" . $data</td><td>unsupported</td></tr>
  <tr><td>1 + n</td><td>1</td><td>"~" ROWSPAN specification</td><td>unsupported</td></tr>
  <tr><td>1 + n</td><td>even</td><td>"~" ROWSPAN specification</td><td>unsupported</td></tr>
  <tr><td>1 + n</td><td>odd, after first</td><td>"Dev&lt;br>" . $data</td><td>unsupported</td></tr>
  <tr><td>2 + n</td><td>1</td><td>"~" ROWSPAN specification</td><td>unsupported</td></tr>
  <tr><td>2 + n</td><td>even</td><td>"Jitter" label</td><td>unsupported</td></tr>
  <tr><td>2 + n</td><td>odd, after first</td><td>"Mean&lt;br>" . $data</td><td>unsupported</td></tr>
  <tr><td>3 + n</td><td>1</td><td>"~" ROWSPAN specification</td><td>unsupported</td></tr>
  <tr><td>3 + n</td><td>even</td><td>"~" ROWSPAN specification</td><td>unsupported</td></tr>
  <tr><td>3 + n</td><td>odd, after first</td><td>"Dev&lt;br>" . $data</td><td>unsupported</td></tr>
  <tr><td>4 + n</td><td>1</td><td>"~" ROWSPAN specification</td><td>unsupported</td></tr>
  <tr><td>4 + n</td><td>even</td><td>">" COLSPAN specification</td><td>unsupported</td></tr>
  <tr><td>4 + n</td><td>odd, after first</td><td>"[[plot]|" . $data . "]]"</td><td>unsupported</td></tr>
</table>

=end html

=for text
      Row     n, Column 1: transport identification (name)
      Row     n, Even columns: "Latency" label
      Row     n, Odd columns: "Mean<br>" . $data
      Row 1 + n, Column 1: "~" ROWSPAN specification
      Row 1 + n, Even columns: "~" ROWSPAN specification
      Row 1 + n, Odd columns: "Dev<br>" . $data
      Row 2 + n, Column 1: "~" ROWSPAN specification
      Row 2 + n, Even columns: "Jitter" label
      Row 2 + n, Odd columns: "Mean<br>" . $data
      Row 3 + n, Column 1: "~" ROWSPAN specification
      Row 3 + n, Even columns: "~" ROWSPAN specification
      Row 3 + n, Odd columns: "Dev<br>" . $data
      Row 4 + n, Column 1: "~" ROWSPAN specification
      Row 4 + n, Even columns: ">" COLSPAN specification
      Row 4 + n, Odd columns: "[[plot|" . $data . "]]"

=for text

Where the mean and deviation data are taken from the input file, and the
plot data is formed from the transport type and message size obtained
from the input file.

=head1 EXAMPLE

  mktable.pl data/latency.csv > doc/results-tiddler

  mktable.pl html data/latency.csv > table-frag.html

=cut

# data - parsed input: $data->{transport}->{size}->{stat}
our ($data, $outputStyle);

# Extract the output style selection from the command line, if any.
BEGIN {
  if( $ARGV[0] eq "html") {
    shift;
    $outputStyle = "html";

  } else {
    $outputStyle = "tiddler";
  }
}

# Skip comments.
next if /#/;

# Parse the CSV input file, ignoring blank lines and removing the line end.
my @fields = split ',';
next if $#fields eq 0;
chomp $fields[9];
$data->{ $fields[0]}->{ $fields[1]}->{ latencymean} = $fields[2];
$data->{ $fields[0]}->{ $fields[1]}->{ latencydev}  = $fields[3];
# $data->{ $fields[0]}->{ $fields[1]}->{ latencymax}  = $fields[4];
# $data->{ $fields[0]}->{ $fields[1]}->{ latencymin}  = $fields[5];
$data->{ $fields[0]}->{ $fields[1]}->{ jittermean}  = $fields[6];
$data->{ $fields[0]}->{ $fields[1]}->{ jitterdev}   = $fields[7];
# $data->{ $fields[0]}->{ $fields[1]}->{ jittermax}   = $fields[8];
# $data->{ $fields[0]}->{ $fields[1]}->{ jittermin}   = $fields[9];

# Create the table using all available information.
END {
  # Lookup table for transport labels.
  my $transportLabels = {
    tcp  => "TCP<br>(reliable)",
    udp  => "UDP<br>(best effort)",
    mbe  => "Multicast<br>(best effort)",
    mrel => "Multicast<br>(reliable)",
    rtps  => "RTPS<br>(reliable)",
  };

  # We will need to know how many columns we are generating before we
  # start.
  my $sizes = 0;
  my $sizeTransport;
  foreach my $transport (keys %$data) {
    my $size = scalar keys %{$data->{$transport}};
    if( $size > $sizes) {
      # Assume that when we process, we can use the sizes from the
      # first found largest row.
      $sizes = $size;
      $sizeTransport = $transport;
    }
  }

  # List of sizes to use.
  my @sizeList = sort { $a <=> $b; } keys %{$data->{$sizeTransport}};

  # Generate the desired output.
  if( $outputStyle eq "html") {
    &htmltable( \@sizeList, $transportLabels);

  } else {
    &tiddlertable( \@sizeList, $transportLabels);
  }
}

sub tiddlertable {
  my ($sizeList, $transportLabels) = @_;

  # Row 1
  print "| Transport |";
  for( my $index = 0; $index < ($#$sizeList - 1); ++$index) {
    print ">|>|";
  }
  print ">| Message Size (bytes) |\n";

  # Row 2
  print "|~|";
  foreach my $size (@$sizeList) {
    print ">| $size |";
  }
  print "\n";

  # Each transport generates 5 rows of data.
  foreach my $transport (sort keys %$data) {
    # Row n
    #   Column 1
    print "| $transportLabels->{$transport} |";
    foreach my $size (@$sizeList) {
      # Even Columns
      print " Latency |";
      # Odd Columns
      print " Mean<br>$data->{$transport}->{$size}->{latencymean} |";
    }
    print "\n";

    # Row 1 + n
    #   Column 1
    print "|~|";
    foreach my $size (@$sizeList) {
      # Even Columns
      print "~|";
      # Odd Columns
      print " Dev<br>$data->{$transport}->{$size}->{latencydev} |";
    }
    print "\n";

    # Row 2 + n
    #   Column 1
    print "|~|";
    foreach my $size (@$sizeList) {
      # Even Columns
      print " Jitter |";
      # Odd Columns
      print " Mean<br>$data->{$transport}->{$size}->{jittermean} |";
    }
    print "\n";

    # Row 3 + n
    #   Column 1
    print "|~|";
    foreach my $size (@$sizeList) {
      # Even Columns
      print "~|";
      # Odd Columns
      print " Dev<br>$data->{$transport}->{$size}->{jitterdev} |";
    }
    print "\n";

    # Row 4 + n
    #   Column 1
    print "|~|";
    foreach my $size (@$sizeList) {
      # Even Columns
      print ">|";
      # Odd Columns
      print " [[plot|$transport-$size]] |";
    }
    print "\n";
  }
}

sub htmltable {
  print "\n\n   HTML output not currently implemented.\n\n\n";
}

