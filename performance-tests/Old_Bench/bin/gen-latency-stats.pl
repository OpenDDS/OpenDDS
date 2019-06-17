eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;
use File::Basename;

=head1 NAME

gen-latency-stats.pl - create GNUPlot statistical summary data string variables


=head1 SYNOPSIS

  gen-latency-stats.pl <infile>

=head1 DESCRIPTION

This script processes a comma separated value (CSV) input file and
creates an output file for each record of the input.  Each output file is
placed in the same directory that the input file was located in.  The
output file names are constructed using fields from the input record as:

  latency-<transport>-<size>.stats

The input file is expected to be in the format produced by the extract.pl
data reduction script.  Each record (line) of the input file contains the
following fields:

=begin html

<table border>
  <tr><th>Field</th><th>Description</th></tr>
  <tr><td>1</td><td>transport type (derived from input filename)</td></tr>
  <tr><td>2</td><td>test message size (derived from input filename)</td></tr>
  <tr><td>3</td><td>latency mean statistic</td></tr>
  <tr><td>4</td><td>latency standard deviation statistic</td></tr>
  <tr><td>5</td><td>latency median statistic</td></tr>
  <tr><td>6</td><td>latency median absolute deviation statistic</td></tr>
  <tr><td>7</td><td>latency maximum statistic</td></tr>
  <tr><td>8</td><td>latency minimum statistic</td></tr>
  <tr><td>9</td><td>jitter mean statistic</td></tr>
  <tr><td>10</td><td>jitter standard deviation statistic</td></tr>
  <tr><td>11</td><td>jitter median statistic</td></tr>
  <tr><td>12</td><td>jitter median absolute deviation statistic</td></tr>
  <tr><td>13</td><td>jitter maximum statistic</td></tr>
  <tr><td>14</td><td>jitter minimum statistic</td></tr>
</table>

=end html

=for text
      Field  1: transport type (derived from input filename)
      Field  2: test message size (derived from input filename)
      Field  3: latency mean statistic
      Field  4: latency standard deviation statistic
      Field  5: latency median statistic
      Field  6: latency median absolute deviation statistic
      Field  7: latency maximum statistic
      Field  8: latency minimum statistic
      Field  9: jitter mean statistic
      Field 10: jitter standard deviation statistic
      Field 11: jitter median statistic
      Field 12: jitter median absolute deviation statistic
      Field 13: jitter maximum statistic
      Field 14: jitter minimum statistic

=for text

The output includes two GNUPlot string variable definitions suitable for
'load' or 'call' operations within GNUPlot.  Provided GNUPlot data
visualization scripts use these variables to generate label information
to place on plots.  The variables are:

  latency_stats
  jitter_stats

Each variable contains the median, median absolute deviation, maximum,
and minimum data values for the output file (transport/size) in a newline
separated single string suitable for use as a label within GNUPlot.

=head1 EXAMPLE

  gen-latency-stats.pl data/latency.csv

=cut

# Skip comments.
next if /#/;

# Parse the CSV input file, ignoring blank lines and removing the line end.
my @fields = split ',';
next if $#fields eq 0;
chomp $fields[13];

# Establish the output file using the transport and size information.
my $filename = dirname($ARGV) . "/" . "latency-" . $fields[0] . "-" . $fields[1] . ".stats";
die "Unable to open output: $filename - $!"
  if not open( OUT, ">$filename");

# Create the 'latency_stats' GNUPlot string variable.
print OUT "latency_stats=\"";
print OUT "Median: $fields[4],\\n";
print OUT "MAD: $fields[5],\\n";
print OUT "Maximum: $fields[6],\\n";
print OUT "Minimum: $fields[7]\\n\"\n";

# Create the 'jitter_stats' GNUPlot string variable.
print OUT "jitter_stats=\"";
print OUT "Median: $fields[10],\\n";
print OUT "MAD: $fields[11],\\n";
print OUT "Maximum: $fields[12],\\n";
print OUT "Minimum: $fields[13]\\n\"\n";

# Current file is done.
close(OUT);

