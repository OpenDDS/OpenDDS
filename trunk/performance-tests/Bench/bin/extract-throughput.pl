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

=head1 EXAMPLE

  extract-throughput.pl */*.data > data/throughput.csv

=cut

# Filename extension to elide.
use constant EXTENSION => ".data";

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
/Publication::enable\(\) - publication \S+: (created|obtained) (.+) transport with index \d+\.$/ and do { $transport = $2; };

# No need to examine data if we have not found required information.
next if ((not defined $transport) or (not defined $rate) or (not defined $size));

# Store data as it is recognized.
/DataReaderListener::on_data_available\(\) - received a non-data sample.  After messages: total: \d+, valid: (\d+)./ and do {$data->{ $transport}->{ $size}->{ $rate}->{actual} = $1; };
/test elapsed time: (\d+\.?\d+)/ and do { $data->{ $transport}->{ $size}->{ $rate}->{time} = $1; };


END {
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
        my $actual = ($data->{$transport}->{$size}->{$rate}->{actual} * $size / $data->{ $transport}->{ $size}->{ $rate}->{time});
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
