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
# test      - current test number being processed.
our ($current, $transport, $size, $rate, $time, $data, $test);

if( not defined $current or $ARGV ne "$current") {
  # Starting a new file.
  $current = $ARGV;
  undef $size;
  undef $rate;
  undef $transport;
  undef $time;
  undef $test;
  if ($current =~ /bidir(\d+)-.*/){
    $test = $1;
  }
#  elsif ($current =~ /test(\d+)-.*/) {
#    $test = $1 + 13;
#  }
}


/Options::loadPublication\(\) -   \[publication\/\S+] MessageRate == (\d+)\.$/ and do { $rate = $1; };
/Options::loadPublication\(\) -   \[publication\/\S+] MessageSize == (\d+)\.$/ and do { $size = $1; };
/::enable\(\) - (publication|subscription) \S+: obtained (.+) transport with index \d+\.$/ and do { $transport = $2; };

# No need to examine data if we have not found required information.
next if ((not defined $transport) or (not defined $test));


# Store data as it is recognized.
/DataReaderListener::on_data_available\(\) - received a non-data sample.  After messages: total: \d+, valid: (\d+)./ and do {$data->{ $transport}->{ $test}->{actual} = $1; };
/^Valid Messages Received: (\d+)$/ and do {$data->{ $transport}->{ $test}->{actual} = $1; };
/test elapsed time: (\d+\.?\d+)/ and do {
  $data->{ $transport}->{ $test}->{time} = $1;
  $data->{ $transport}->{ $test}->{size} = $size;
  $data->{ $transport}->{ $test}->{rate} = $rate;
};

sub printtestresult ($$$$) {
  my $size = shift;
  my $rate = shift;
  my $messages = shift;
  my $time = shift;
  my $actual = $messages * $size * 8 / $time;
  print "$size $rate $actual\n";
}


END {
  die "No actual data gathered." if not $data or not scalar keys %$data;

  my $index = 0;
  foreach my $transport (sort keys %$data) {
    print "#\n";
    print "# Index " . $index++ . " data for $transport\n";
    print "#\n";
    print "# size (bytes per sample),\n";
    print "# rate (samples per second),\n";
    print "# throughput per second (bits per second)\n";
    print "#\n";
    print "# expected throughput per second == size * rate * 8\n";
    print "#  -- using x:(column(1)*column(2)*8))\n";
    print "#\n";
    print "# Bidirectional / steepest ascent\n";
    printtestresult ($data->{ $transport}->{ 1}->{size}, $data->{ $transport}->{ 1}->{rate}, $data->{ $transport}->{ 1}->{actual}, $data->{ $transport}->{ 1}->{time});
    printtestresult ($data->{ $transport}->{ 2}->{size}, $data->{ $transport}->{ 2}->{rate}, $data->{ $transport}->{ 2}->{actual}, $data->{ $transport}->{ 2}->{time});
    printtestresult ($data->{ $transport}->{ 3}->{size}, $data->{ $transport}->{ 3}->{rate}, $data->{ $transport}->{ 3}->{actual}, $data->{ $transport}->{ 3}->{time});
    printtestresult ($data->{ $transport}->{ 4}->{size}, $data->{ $transport}->{ 4}->{rate}, $data->{ $transport}->{ 4}->{actual}, $data->{ $transport}->{ 4}->{time});
    printtestresult ($data->{ $transport}->{ 5}->{size}, $data->{ $transport}->{ 5}->{rate}, $data->{ $transport}->{ 5}->{actual}, $data->{ $transport}->{ 5}->{time});
    print "\n\n";

    print "#\n";
    print "# Index " . $index++ . " data for $transport\n";
    print "#\n";
    print "# size (bytes per sample),\n";
    print "# rate (samples per second),\n";
    print "# throughput per second (bits per second)\n";
    print "#\n";
    print "# expected throughput per second == size * rate * 8\n";
    print "#  -- using x:(column(1)*column(2)*8))\n";
    print "#\n";
    print "# Bidirectional / fixed rate\n";
    printtestresult ($data->{ $transport}->{ 1}->{size}, $data->{ $transport}->{ 1}->{rate}, $data->{ $transport}->{ 1}->{actual}, $data->{ $transport}->{ 1}->{time});
    printtestresult ($data->{ $transport}->{ 6}->{size}, $data->{ $transport}->{ 6}->{rate}, $data->{ $transport}->{ 6}->{actual}, $data->{ $transport}->{ 6}->{time});
    printtestresult ($data->{ $transport}->{ 7}->{size}, $data->{ $transport}->{ 7}->{rate}, $data->{ $transport}->{ 7}->{actual}, $data->{ $transport}->{ 7}->{time});
    printtestresult ($data->{ $transport}->{ 8}->{size}, $data->{ $transport}->{ 8}->{rate}, $data->{ $transport}->{ 8}->{actual}, $data->{ $transport}->{ 8}->{time});
    printtestresult ($data->{ $transport}->{ 9}->{size}, $data->{ $transport}->{ 9}->{rate}, $data->{ $transport}->{ 9}->{actual}, $data->{ $transport}->{ 9}->{time});
    print "\n\n";

    print "#\n";
    print "# Index " . $index++ . " data for $transport\n";
    print "#\n";
    print "# size (bytes per sample),\n";
    print "# rate (samples per second),\n";
    print "# throughput per second (bits per second)\n";
    print "#\n";
    print "# expected throughput per second == size * rate * 8\n";
    print "#  -- using x:(column(1)*column(2)*8))\n";
    print "#\n";
    print "# Bidirectional / fixed size\n";
    printtestresult ($data->{ $transport}->{ 1}->{size}, $data->{ $transport}->{ 1}->{rate}, $data->{ $transport}->{ 1}->{actual}, $data->{ $transport}->{ 1}->{time});
    printtestresult ($data->{ $transport}->{ 10}->{size}, $data->{ $transport}->{ 10}->{rate}, $data->{ $transport}->{ 10}->{actual}, $data->{ $transport}->{ 10}->{time});
    printtestresult ($data->{ $transport}->{ 11}->{size}, $data->{ $transport}->{ 11}->{rate}, $data->{ $transport}->{ 11}->{actual}, $data->{ $transport}->{ 11}->{time});
    printtestresult ($data->{ $transport}->{ 12}->{size}, $data->{ $transport}->{ 12}->{rate}, $data->{ $transport}->{ 12}->{actual}, $data->{ $transport}->{ 12}->{time});
    printtestresult ($data->{ $transport}->{ 13}->{size}, $data->{ $transport}->{ 13}->{rate}, $data->{ $transport}->{ 13}->{actual}, $data->{ $transport}->{ 13}->{time});
    print "\n\n";
  }
}
