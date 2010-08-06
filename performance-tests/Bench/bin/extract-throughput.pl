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
# type      - type of current test being processed.
our ($current, $transport, $size, $rate, $time, $data, $test, $type);

BEGIN {
  # Seed the test types and the transports so that we can maintain a
  # consistent set of indices in the output for our plotting scripts.
  $data = { bidir =>    { 'SimpleTcp'             => undef,
                          'best effort multicast' => undef,
                          'reliable multicast'    => undef,
                          'udp'                   => undef},
            pubbound => { 'SimpleTcp'             => undef,
                          'best effort multicast' => undef,
                          'reliable multicast'    => undef,
                          'udp'                   => undef},
            subbound => { 'SimpleTcp'             => undef,
                          'best effort multicast' => undef,
                          'reliable multicast'    => undef,
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
/::enable\(\) - (publication|subscription) \S+: obtained (.+) transport with index \d+\.$/ and do { $transport = $2; };

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
      $type =~ /bidir/    && do { $scale = 16; $groups = &testGroups($type); last SWITCH; };
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

