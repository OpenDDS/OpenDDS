eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;
use File::Basename;

=head1 NAME

extract-latency-json.pl - extract summary statistics from plot datafiles.


=head1 SYNOPSIS

  extract-latency.pl <infile> ...

=head1 DESCRIPTION

This script processes the input files and prints a summary from all files
in comma separated values (CSV) format to standard output.

The input file is expected to be in the format produced by the
reduce-latency-data.pl data reduction script.  This file type has
statistical summary data in the Index 1 and Index 2 section header
comments that are parsed by this script and gathered from all input files.

This input file name is expected to be in a format that includes '-'
separated fields and a fixed extension of ".gpd".

  latency-<transport>-<size>.gpd

The <transport> and <size> fields are used to populate two columns in the
output data.

This output consists of a single CSV file with a single record (line)
generated from each input file.  The output lines include:

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

=head1 EXAMPLE

  extract-latency.pl data/*.gpd > data/latency.csv

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
  my $testtype;
  my $slot4;
  ($testtype, $transport, $size , $slot4) = split "-", basename( $ARGV, EXTENSION);
  if ($slot4) {
      $transport .= "-$size";
      $size = $slot4;
  }
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
/^#\s+Median: (\S+)/    && do { $key = $section . "median"; };
/^#\s+MAD: (\S+)/       && do { $key = $section . "MAD"; };
/^#\s+Maximum: (\S+)/   && do { $key = $section . "max"; };
/^#\s+Minimum: (\S+)/   && do { $key = $section . "min"; };
$data->{ $transport}->{ $size}->{ $key } = $1 if $key;

END {
  # Format output as JSON data.
  print "const columns=[\n";
  print "\"transport\", \"size\", \"latency mean\", \"latency std. dev\",\n";
  print " \"latency median\", \"latency median absolute deviation\",\n";
  print " \"latency max\", \"latency min\", \"jitter mean\", \"jitter dev\",\n";
  print " \"jitter median\", \"jitter median absolute deviation\",\n";
  print " \"jitter max\", \"jitter min\"\n";
  print "];\n\n";
  my $block = 0;
  my $index = 0;
  print "const latency_data=[";
  foreach my $transport (sort keys %$data) {
    if ($block++) {
      print "`,\n";
    }
    print "\n`";
    $index = 0;
    foreach my $size (sort { $a <=> $b; } keys %{$data->{$transport}}) {
      if ($index++) {
        print "\n";
      }
      print "$transport,";
      print "$size,";
      print "$data->{$transport}->{$size}->{latencymean},";
      print "$data->{$transport}->{$size}->{latencydev},";
      print "$data->{$transport}->{$size}->{latencymedian},";
      print "$data->{$transport}->{$size}->{latencyMAD},";
      print "$data->{$transport}->{$size}->{latencymax},";
      print "$data->{$transport}->{$size}->{latencymin},";
      print "$data->{$transport}->{$size}->{jittermean},";
      print "$data->{$transport}->{$size}->{jitterdev},";
      print "$data->{$transport}->{$size}->{jittermedian},";
      print "$data->{$transport}->{$size}->{jitterMAD},";
      print "$data->{$transport}->{$size}->{jittermax},";
      print "$data->{$transport}->{$size}->{jittermin}";
    }
  }
  print "`];\n";
}

