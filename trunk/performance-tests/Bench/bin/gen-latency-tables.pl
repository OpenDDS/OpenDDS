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
chomp $fields[9];

my $transport = $fields[0];
my $size = $fields[1];
$data->{$transport}->{$size}->{latencymean} = $fields[2];
$data->{$transport}->{$size}->{latencydev} = $fields[3];
$data->{$transport}->{$size}->{latencymedian} = $fields[4];
$data->{$transport}->{$size}->{latencyMAD} = $fields[5];
$data->{$transport}->{$size}->{latencymax} = $fields[6];
$data->{$transport}->{$size}->{latencymin} = $fields[7];
$data->{$transport}->{$size}->{jittermean} = $fields[8];
$data->{$transport}->{$size}->{jitterdev} = $fields[9];
$data->{$transport}->{$size}->{jittermedian} = $fields[10];
$data->{$transport}->{$size}->{jitterMAD} = $fields[11];
$data->{$transport}->{$size}->{jittermax} = $fields[12];
$data->{$transport}->{$size}->{jittermin} = $fields[13];

sub transportName {
  # Lookup table for transport types.
  my $type = shift;
  return "UNKNOWN TRANSPORT TYPE"  if not $type;
  return "TCP"      if $type eq "tcp";
  return "TCP"      if $type eq "SimpleTcp";
  return "Multicast Best Effort"  if $type eq "mbe";
  return "Multicast Best Effort"  if $type eq "best effort multicast";
  return "Multicast Reliable" if ($type eq "mre" || $type eq "mrel");
  return "Multicast Reliable" if $type eq "reliable multicast";
  return "UDP" if $type eq "udp";
  return "RTPS" if $type eq "rtps";
  return "UNKNOWN TRANSPORT TYPE";
}

END {

# Establish the output file using the transport and size information.
my $lat_filename = dirname($ARGV) . "/" . "latency-html-tables.txt";
die "Unable to open output: $lat_filename - $!"
  if not open( LATOUT, ">$lat_filename");
my $jit_filename = dirname($ARGV) . "/" . "jitter-html-tables.txt";
die "Unable to open output: $jit_filename - $!"
  if not open( JITOUT, ">$jit_filename");
my $nj_filename = dirname($ARGV) . "/" . "normaljitter-html-tables.txt";
die "Unable to open output: $nj_filename - $!"
  if not open( NMJOUT, ">$nj_filename");

  print LATOUT "<div class=\"data_tables\">\n" .
               "<h2 class=\"data_tables\">Latency Data<a name=\"latency_tables\">&nbsp;</a></h2>\n" .
               "<hr class=\"data_tables\" />\n";

  print JITOUT "<div class=\"data_tables\">\n" .
               "<h2 class=\"data_tables\">Jitter Data<a name=\"jitter_tables\">&nbsp;</a></h2>\n" .
               "<hr class=\"data_tables\" />\n";

  print NMJOUT "<div class=\"data_tables\">\n" .
               "<h2 class=\"data_tables\">Normalized Jitter Data<a name=\"normal_jitter_tables\">&nbsp;</a></h2>\n" .
               "<p>Jitter was normalized to the average latency (mean) for the message size-transport combination.  The values are percent of the average latency.</p>" .
               "<hr class=\"data_tables\" />\n";

  foreach my $transport (sort keys %$data) {

    print LATOUT "<h3 class=\"data_tables\">" . transportName($transport) . "</h3>\n";
    print LATOUT "<table class=\"data_tables\">\n";
    print LATOUT "<tr class=\"data_tables\">" .
                 "<th class=\"data_tables\">Message Size</th>" .
                 "<th class=\"data_tables\">Mean</th>" .
                 "<th class=\"data_tables\">Std. Dev.</th>" .
                 "<th class=\"data_tables\">Median</th>" .
                 "<th class=\"data_tables\">Median Abs. Dev.</th>" .
                 "<th class=\"data_tables\">Min</th>" .
                 "<th class=\"data_tables\">Max</th></tr>\n";

    print JITOUT "<h3 class=\"data_tables\">" . transportName($transport) . "</h3>\n";
    print JITOUT "<table class=\"data_tables\">\n";
    print JITOUT "<tr class=\"data_tables\">" .
                 "<th class=\"data_tables\">Message Size</th>" .
                 "<th class=\"data_tables\">Mean</th>" .
                 "<th class=\"data_tables\">Std. Dev.</th>" .
                 "<th class=\"data_tables\">Median</th>" .
                 "<th class=\"data_tables\">Median Abs. Dev.</th>" .
                 "<th class=\"data_tables\">Min</th>" .
                 "<th class=\"data_tables\">Max</th></tr>\n";

    print NMJOUT "<h3 class=\"data_tables\">" . transportName($transport) . "</h3>\n";
    print NMJOUT "<table class=\"data_tables\">\n";
    print NMJOUT "<tr class=\"data_tables\">" .
                 "<th class=\"data_tables\">Message Size</th>" .
                 "<th class=\"data_tables\">Norm. Std. Dev.</th>" .
                 "<th class=\"data_tables\">Norm. Min</th>" .
                 "<th class=\"data_tables\">Norm. Max</th></tr>\n";

    foreach my $size (sort { $a <=> $b; } keys %{$data->{$transport}}) {
      print LATOUT "<tr class=\"data_tables\">" .
                   "<td class=\"data_tables\">$size</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{latencymean}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{latencydev}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{latencymedian}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{latencyMAD}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{latencymin}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{latencymax}</td></tr>\n";

      print JITOUT "<tr class=\"data_tables\">" .
                   "<td class=\"data_tables\">$size</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{jittermean}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{jitterdev}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{jittermedian}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{jitterMAD}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{jittermin}</td>" .
                   "<td class=\"data_tables\">$data->{$transport}->{$size}->{jittermax}</td></tr>\n";

      print NMJOUT "<tr class=\"data_tables\">" .
                   "<td class=\"data_tables\">$size</td>" .
                   "<td class=\"data_tables\">" . sprintf("%.2e", 100 * $data->{$transport}->{$size}->{jitterdev} / $data->{$transport}->{$size}->{latencymean}) . "</td>" .
                   "<td class=\"data_tables\">" . sprintf("%.2e", 100 * $data->{$transport}->{$size}->{jittermin} / $data->{$transport}->{$size}->{latencymean}) . "</td>" .
                   "<td class=\"data_tables\">" . sprintf("%.2e", 100 * $data->{$transport}->{$size}->{jittermax} / $data->{$transport}->{$size}->{latencymean}) . "</td></tr>\n";    }
    print LATOUT "</table>\n\n";
    print JITOUT "</table>\n\n";
    print NMJOUT "</table>\n\n";
  }

  print LATOUT "</div>\n";
  print JITOUT "</div>\n";
  print NMJOUT "</div>\n";

  close(LATOUT);
  close(JITOUT);
  close(NMJOUT);
}
