eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;

=head1 NAME

extract-throughput.pl - extract summary statistics from test results.


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

The supported tests that can be reduced from the pre-configured throughput
tests:

=begin html

<table border>
  <tr><th>Test Type</th></tr>
  <tr><td>Bidirectional Throughput</td></tr>
  <tr><td>Publication Bound</td></tr>
  <tr><td>Subscription Bound</td></tr>
</table>

=end html

=begin text

      Bidirectional Throughput
      Publication Bound
      Subscription Bound

=end text

=begin man

      Bidirectional Throughput
      Publication Bound
      Subscription Bound

=end man

Each of these test types has a pre-configured set of test conditions that
can be executed.  The sets of test conditions are grouped into three
different categories:

=begin html

<table border>
  <tr><th>Test Conditions</th><th>Description</th></tr>
  <tr><td>Steepest Ascent</td><td>A group of tests that simultaneously
  increase both the message size and message rate resulting in a large
  nominal throughput increase between tests.</td></tr>
  <tr><td>Fixed Rate</td><td>A group of tests where the message rate is
  held constant for all of the tests.</td></tr>
  <tr><td>Fixed Size</td><td>A group of tests where the message size is
  held constant for all of the tests.</td></tr>
</table>

=end html

=begin text

      Steepest Ascent: A group of tests that simultaneously increase both
                       the message size and message rate resulting in a
                       large nominal throughput increase between tests.
      Fixed Rate:      A group of tests where the message rate is held
                       constant for all of the tests.
      Fixed Size:      A group of tests where the message size is held
                       constant for all of the tests.

=end text

=begin man

      Steepest Ascent: A group of tests that simultaneously increase both
                       the message size and message rate resulting in a
                       large nominal throughput increase between tests.
      Fixed Rate:      A group of tests where the message rate is held
                       constant for all of the tests.
      Fixed Size:      A group of tests where the message size is held
                       constant for all of the tests.

=end man

The transport types included in the pre-configured tests are:

=begin html

<table border>
  <tr><th>Transport</th><th>Description</th></tr>
  <tr><td>UDP</td><td>best effort datagram transport.</td></tr>
  <tr><td>TCP</td><td>reliable stream transport.</td></tr>
  <tr><td>Best Effort Multicast</td><td>best effort multicast datagram transport.</td></tr>
  <tr><td>Reliable Multicast</td><td>reliable multicast datagram transport.</td></tr>
  <tr><td>RTPS</td><td>real-time publish-subscribe transport.</td></tr>
</table>

=end html

=begin text

      UDP:                   best effort datagram transport.
      TCP:                   reliable stream transport.
      Best Effort Multicast: best effort multicast datagram transport.
      Reliable Multicast:    reliable multicast datagram transport.
      RTPS:                  real-time publish-subscribe transport.

=end text

=begin man

      UDP:                   best effort datagram transport.
      TCP:                   reliable stream transport.
      Best Effort Multicast: best effort multicast datagram transport.
      Reliable Multicast:    reliable multicast datagram transport.
      RTPS:                  real-time publish-subscribe transport.

=end man

Each index set in the output represents the combination of a test type,
a set of test conditions, and a transport type with each entry within
the index representing a single size/rate test result.

The output data within an index is formatted as a comma separated value
file (CSV), with the following fields:

=begin html

<table border>
  <tr><th>Field</th><th>Description</th></tr>
  <tr><td>1</td><td>test message size</td></tr>
  <tr><td>2</td><td>test message rate</td></tr>
  <tr><td>3</td><td>actual (measured) bandwidth</td></tr>
  <tr><td>4</td><td>test type</td></tr>
  <tr><td>5</td><td>transport type</td></tr>
</table>

=end html

=begin text

      Field  1: test message size
      Field  2: test message rate
      Field  3: actual (measured) bandwidth
      Field  4: test type
      Field  5: transport type

=end text

=begin man

      Field  1: test message size
      Field  2: test message rate
      Field  3: actual (measured) bandwidth
      Field  4: test type
      Field  5: transport type

=end man

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
# type      - type of current test being processed.
our ($current, $transport, $size, $rate, $time, $data, $test, $type);

BEGIN {
  # Seed the test types and the transports so that we can maintain a
  # consistent set of indices in the output for our plotting scripts.
  $data = { bidir =>    { 'tcp'                   => undef,
                          'best effort multicast' => undef,
                          'reliable multicast'    => undef,
                          'rtps_udp'              => undef,
                          'udp'                   => undef},
            pubbound => { 'tcp'                   => undef,
                          'best effort multicast' => undef,
                          'reliable multicast'    => undef,
                          'rtps'                  => undef,
                          'udp'                   => undef},
            subbound => { 'tcp'                   => undef,
                          'best effort multicast' => undef,
                          'reliable multicast'    => undef,
                          'rtps_udp'              => undef,
                          'udp'                   => undef}
          };
}

if( not defined $current or $ARGV ne "$current") {
  # Starting a new file.
  $current = $ARGV;
  undef $size;
  undef $rate;
  undef $transport;
  undef $time;
  undef $test;
  if ($current =~ /bidir(\d+).*/){
    $test = $1;
    $type = "bidir";
  }
 elsif ($current =~ /test(\d+)-(pub|sub)(\d*)/) {
   $test = $1;
   if( $2 eq "pub") {
     $type = "pubbound";
     $type = "subbound" if $3;
   } elsif( $2 eq "sub") {
     $type = "subbound";
     $type = "pubbound" if $3;
   }
 }
 # print "$current: $type test $test.\n";
}


/Options::loadPublication\(\) -   \[publication\/\S+] MessageRate == (\d+)\.$/ and do { $rate = $1; };
/Options::loadPublication\(\) -   \[publication\/\S+] MessageSize == (\d+)\.$/ and do { $size = $1; };
/::enable\(\) - (publication|subscription) \S+: obtained (.+) transport with (index|config) \d+\.$/ and do { $transport = $2; };

# No need to examine data if we have not found required information.
next if ((not defined $transport) or (not defined $test));


# Store data as it is recognized.
/DataReaderListener::on_data_available\(\) - received a non-data sample.  After messages: total: \d+, valid: (\d+)./ and do {$data->{ $type}->{ $transport}->{ $test}->{actual} += $1; };
/^Valid Messages Received: (\d+)$/ and do {$data->{ $type}->{ $transport}->{ $test}->{actual} = $1; };
/test elapsed time: (\d+\.?\d+)/ and do {
  $data->{ $type}->{ $transport}->{ $test}->{time} = $1;
  $data->{ $type}->{ $transport}->{ $test}->{size} = $size;
  $data->{ $type}->{ $transport}->{ $test}->{rate} = $rate;
};

sub testName {
  # Lookup table for test types.
  my $type = shift;
  return "UNKNOWN TEST TYPE"  if not $type;
  return "Bidirectional"      if $type eq "bidir";
  return "Publication Bound"  if $type eq "pubbound";
  return "Subscription Bound" if $type eq "subbound";
  return "UNKNOWN TEST TYPE";
}

sub testGroups {
  # Lookup table for test numbers.
  return { bidir    => [ [  1,  2,  3,  4,  5], [  1,  6,  7,  8,  9], [  1, 10, 11, 12, 13]],
           pubbound => [ [  1,  2,  3,  4,  5], [  1,  6,  7,  8,  9], [  1, 10, 11, 12, 13]],
           subbound => [ [ 14, 15, 16, 17, 18], [ 19, 20, 21, 15, 22], [ 14, 20, 23, 24, 25]]}->{ $_[0]};
}

sub print_header {
  my $index     = shift;
  my $transport = shift || "UNKNOWN TRANSPORT";
  my $type      = shift || "UNKNOWN TEST TYPE";
  my $subtype   = shift || "UNKNOWN TEST SUBTYPE";
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
  print "# $type / $subtype\n";
}

sub dataline {
  my $dataline  = shift;
  my $scale     = shift || 8;
  my $type      = shift || "NoType";
  my $transport = shift || "NoTransport";
  return "# NO DATA\n"        if not $dataline;
  return "# NO SIZE\n"        if not $dataline->{size};
  return "# NO RATE\n"        if not $dataline->{rate};
  return "# NO MEASUREMENT\n" if not $dataline->{actual};
  return "$dataline->{size},$dataline->{rate},"
       . ($dataline->{actual} * $dataline->{size} * $scale / ($dataline->{time} || 120))
       . ",$type,$transport\n";
}

END {
  die "No actual data gathered." if not $data or not scalar keys %$data;

  my $index = 0;

  # Process each test type in turn.
  foreach my $type (sort keys %$data) {
    my $scale;
    my $groups;
    SWITCH:{
      # bidir scale depend on whether the connections are full duplex 8 or half duplex 16
      #  the test environment uses full duplex
      #$type =~ /bidir/    && do { $scale = 16; $groups = &testGroups($type); last SWITCH; };
      $type =~ /bidir/    && do { $scale =  8; $groups = &testGroups($type); last SWITCH; };
      $type =~ /pubbound/ && do { $scale =  8; $groups = &testGroups($type); last SWITCH; };
      $type =~ /subbound/ && do { $scale =  8; $groups = &testGroups($type); last SWITCH; };
    }

    # Then process each transport in turn.
    foreach my $transport (sort keys %{$data->{$type}}) {
      # Each test type has three arrangements.
      for my $current (0 .. 2) {
        my $subtype = [ "steepest ascent", "fixed rate", "fixed size"]->[$current];
        &print_header( $index++, $transport, &testName($type), $subtype);
        foreach my $test (@{$groups->[$current]}) {
          print &dataline( $data->{$type}->{$transport}->{$test}, $scale, $type, $transport);
        }
        print "\n\n";
      }
    }
  }
}
