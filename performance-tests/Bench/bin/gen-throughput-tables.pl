eval '(exit $?0)' && eval 'exec perl -nS $0 ${1+"$@"}'
    & eval 'exec perl -nS $0 $argv:q'
    if 0;

use warnings;
use strict;
use File::Basename;


our ($data);

# Skip comments.
next if /#/;

# Parse the CSV input file, ignoring blank lines and removing the line end.
my @fields = split ',';
next if $#fields eq 0;
chomp $fields[4];

my $size = $fields[0];
my $rate = $fields[1];
my $type = $fields[3];
my $transport = $fields[4];
$data->{$type}->{$transport}->{$size}->{$rate} = $fields[2];


sub typeName {
  # Lookup table for test types.
  my $type = shift;
  return "UNKNOWN TEST TYPE"  if not $type;
  return "Bidirectional"      if $type eq "bidir";
  return "Publication Bound"  if $type eq "pubbound";
  return "Subscription Bound" if $type eq "subbound";
  return "UNKNOWN TEST TYPE";
}

sub transportName {
  # Lookup table for transport types.
  my $type = shift;
  return "UNKNOWN TRANSPORT TYPE"  if not $type;
  return "TCP"      if $type eq "tcp";
  return "TCP"      if $type eq "SimpleTcp";
  return "Multicast Best Effort"  if $type eq "mbe";
  return "Multicast Best Effort"  if $type eq "best effort multicast";
  return "Multicast Reliable" if $type eq "mre";
  return "Multicast Reliable" if $type eq "reliable multicast";
  return "UDP" if $type eq "udp";
  return "RTPS" if $type eq "rtps";
  return "RTPS" if $type eq "rtps_udp";
  return "UNKNOWN TRANSPORT TYPE";
}

END {
my $network_max = 100000000;
my $thru_filename = dirname($ARGV) . "/" . "throughput-html-tables.txt";
die "Unable to open output: $thru_filename - $!"
  if not open( THROUT, ">$thru_filename");

  foreach my $type (sort keys %$data) {
    print THROUT "<div class=\"data_tables\">\n" .
                 "<h2 class=\"data_tables\">" . typeName($type) .
                 " Throughput Data<a name=\"" . $type . "_tables" .
                 "\">&nbsp;</a></h2>\n" .
                 "<hr class=\"data_tables\" />\n";


    foreach my $transport (sort keys %{$data->{$type}}) {
      print THROUT "<h3 class=\"data_tables\">" . transportName($transport) . "</h3>\n";
      print THROUT "<table class=\"data_tables\">\n";
      print THROUT "<tr class=\"data_tables\">" .
                   "<th class=\"data_tables\">Message Size</th>" .
                   "<th class=\"data_tables\">Message Rate</th>" .
                   "<th class=\"data_tables\">Throughput</th>" .
                   "<th class=\"data_tables\">Percent Possible</th></tr>\n";

      foreach my $size (sort { $a <=> $b; } keys %{$data->{$type}->{$transport}}) {
        foreach my $rate (sort { $a <=> $b; } keys %{$data->{$type}->{$transport}->{$size}}) {
          my $test_max = $size * $rate * 8;
          $test_max = $test_max < $network_max ? $test_max : $network_max;
          print THROUT "<tr class=\"data_tables\">" .
                       "<td class=\"data_tables\">$size</td>" .
                       "<td class=\"data_tables\">$rate</td>" .
                       "<td class=\"data_tables\">" . sprintf("%.2e", $data->{$type}->{$transport}->{$size}->{$rate}) . "</td>" .
                       "<td class=\"data_tables\">" .
                       sprintf("%.2e", 100 * $data->{$type}->{$transport}->{$size}->{$rate} / $test_max) . "</td></tr>\n";

        }

      }
      print THROUT "</table>\n\n";
    }

    print THROUT "</div>\n";
  }


  close(THROUT);
}
