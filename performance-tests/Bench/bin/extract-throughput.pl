eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;

=head1 NAME

extract-throughput.pl - extract summary statistics from test results.

$Id$

=head1 SYNOPSIS

  extract-throughput.pl <infile> ...

=head1 DESCRIPTION

This script processes the logfiles created by executing the througput
tests and generates a series of indexed data sets suitable for plotting
by GNUPlot.

All input files named on the command line are processed and the transport
used, the message sizes and rates, as well as the elapsed test time and
actual number of messages received are extracted.

The input files are expected to be in logfile format as produced by the
testprocess C<-v> option.  Logfiles produced by the subscription end of
the testing contain the data required to produce plottable data sets.

Each index set in the output represents one transport type with each
entry within the index representing a single size/rate test result.

The data within an index is formatted as a CSV, with the following
fields:

=begin html

<table border>
  <tr><th>Field</th><th>Description</th></tr>
  <tr><td>1</td><td>transport type</td></tr>
  <tr><td>2</td><td>test message size</td></tr>
  <tr><td>3</td><td>test message rate</td></tr>
  <tr><td>4</td><td>actual (measured) bandwidth</td></tr>
  <tr><td>5</td><td>nominal (specified) bandwidth</td></tr>
</table>

=end html

=for text
      Field  1: transport type
      Field  2: test message size
      Field  3: test message rate
      Field  4: actual (measured) bandwidth
      Field  5: nominal (specified) bandwidth

=head1 EXAMPLE

  extract-throughput.pl */*.results > data/throughput.csv

=cut

# current   - current filename being processed.
# transport - current transport being processed.
# size      - current message size being processed.
# rate      - current message rate being processed.
# time      - current time of test being processed.
# data      - data that has been processed.
our ($current, $transport, $size, $rate, $time, $data);

if( not defined $current or $ARGV ne "$current") {
  # Starting a new file.
  $current = $ARGV;
  undef $size;
  undef $rate;
  undef $transport;
  undef $time;
}

/Options::loadPublication\(\) -   \[publication\/\S+] MessageRate == (\d+)\.$/ and do { $rate = $1; };
/Options::loadPublication\(\) -   \[publication\/\S+] MessageSize == (\d+)\.$/ and do { $size = $1; };
/::enable\(\) - (publication|subscription) \S+: obtained (.+) transport with index \d+\.$/ and do { $transport = $2; };

# No need to examine data if we have not found required information.
next if ((not defined $transport) or (not defined $rate) or (not defined $size));

# Store data as it is recognized.
/DataReaderListener::on_data_available\(\) - received a non-data sample.  After messages: total: \d+, valid: (\d+)./ and do {$data->{ $transport}->{ $size}->{ $rate}->{actual} = $1; };
/test elapsed time: (\d+\.?\d+)/ and do { $data->{ $transport}->{ $size}->{ $rate}->{time} = $1; };


END {
  die "No actual data gathered." if not $data or not scalar keys %$data;

  # Format output as CSV data.
  my $index = 0;
  foreach my $transport (sort keys %$data) {
    print "#\n";
    print "# Index " . $index++ . " data for $transport.\n";
    print "#\n";
    print "# transport, size, rate, throughput per second,\n";
    print "#   expected throughput per second\n";
    print "#\n";
    foreach my $size (sort { $a <=> $b; } keys %{$data->{$transport}}) {
      foreach my $rate (sort { $a <=> $b; } keys %{$data->{$transport}->{$size}}) {
        my $expected = $size * $rate;
        my $elapsed = $data->{ $transport}->{ $size}->{ $rate}->{time};
        next if not $elapsed;
        my $actual = ($data->{$transport}->{$size}->{$rate}->{actual} * $size / $elapsed);
        print "$transport,";
        print "$size,";
        print "$rate,";
        print "$actual,";
        print "$expected";
        print "\n";
      }
    }
    print "\n\n";
  }
}
