eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

# Use information from the environment.
use Env qw( $DDS_ROOT
            $ACE_ROOT
            $TAO_ROOT);

my $date = shift || die "Missing date parameter\n";
my $dds_version;
my $ace_version;
my $tao_version;
my $year = 0;
my $month = 0;
my $day = 0;

@months = ("", "Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec");

$date =~ /(\d{4})(\d{2})(\d{2})/ && do { $year = $1; $month = $2; $day = $3; };


my $dds_release = `head -n1 $DDS_ROOT/VERSION`;
my $dds_current = `head -n1 $DDS_ROOT/ChangeLog`;
if ($dds_release =~ /version ([^,]*), released (\w+)\s+(\w+)\s+(\d+)\s+([\0-9:]+)\s+\w+\s+(\d+)/) {
  $dds_version = $1;
  my $dw = $2;
  my $mon = $3;
  my $day = $4;
  my $time = $5;
  my $year = $6;
  if ($dds_current =~ /^\s*(\w+)\s+(\w+)\s+(\d+)\s+([\0-9:]+)\s+\w+\s+(\d+)/) {
    if ( ($dw ne $1) or ($mon ne $2) or ($day ne $3) or ($time ne $4) or ($year ne $5) ) {
      $dds_version = "Post $dds_version - $2 $3 $4 $5";
    }
  }
}


my $ace_release = `head -n1 $ACE_ROOT/VERSION`;
my $ace_current = `head -n1 $ACE_ROOT/OCIChangeLog`;
if ($ace_release =~ /version ([^,]*), released (\w+)\s+(\w+)\s+(\d+)\s+([\0-9:]+)\s+\w+\s+(\d+)/) {
  $ace_version = $1;
  my $dw = $2;
  my $mon = $3;
  my $day = $4;
  my $time = $5;
  my $year = $6;
  if ($ace_current =~ /^\s*(\w+)\s+(\w+)\s+(\d+)\s+([\0-9:]+)\s+\w+\s+(\d+)/) {
    if ( ($dw ne $1) or ($mon ne $2) or ($day ne $3) or ($time ne $4) or ($year ne $5) ) {
      $ace_version = "Post $ace_version - $2 $3 $4 $5";
    }
  }
}

my $tao_release = `head -n1 $TAO_ROOT/VERSION`;
my $tao_current = `head -n1 $TAO_ROOT/OCIChangeLog`;
if ($tao_release =~ /version ([^,]*), released (\w+)\s+(\w+)\s+(\d+)\s+([\0-9:]+)\s+\w+\s+(\d+)/) {
  $tao_version = $1;
  my $dw = $2;
  my $mon = $3;
  my $day = $4;
  my $time = $5;
  my $year = $6;
  if ($tao_current =~ /^\s*(\w+)\s+(\w+)\s+(\d+)\s+([\0-9:]+)\s+\w+\s+(\d+)/) {
    if ( ($dw ne $1) or ($mon ne $2) or ($day ne $3) or ($time ne $4) or ($year ne $5) ) {
      $tao_version = "Post $tao_version - $2 $3 $4 $5";
    }
  }
}


print "<div class=\"run_info\">\n" .
  "<h2 class=\"run_info\">Run Information<a name=\"rundate\">&nbsp;</a></h2>\n";
print "<table class=\"run_info\">\n";
print "<tr class=\"run_info\"><td class=\"run_info_cell\">Date of Testing:</td><td class=\"run_info_cellvalue\">" .
 $months[$month] . " $day, $year" .
 "</td></tr>\n";

print "<tr class=\"run_info\"><td class=\"run_info_cell\">OpenDDS Version:</td><td class=\"run_info_cellvalue\">";
print $dds_version ? $dds_version : "Unable to find version info";
print "</td></tr>\n";

print "<tr class=\"run_info\"><td class=\"run_info_cell\">TAO Version:</td><td class=\"run_info_cellvalue\">";
print $tao_version ? $tao_version : "Unable to find version info";
print "</td></tr>\n";

print "<tr class=\"run_info\"><td class=\"run_info_cell\">ACE Version:</td><td class=\"run_info_cellvalue\">";
print $ace_version ? $ace_version : "Unable to find version info";
print "</td></tr>\n";

print "<tr class=\"run_info\"><td class=\"run_info_cell\">Log Files:</td><td class=\"run_info_cellvalue\">" .
 "<a class=\"run_data\" href=\"perf_tests_logs_" . $date . ".tar.gz\" alt=\"run logs\">" .
 $date .
 "</a></td></tr>\n";

print "</table>\n";
print "<div>\n";
