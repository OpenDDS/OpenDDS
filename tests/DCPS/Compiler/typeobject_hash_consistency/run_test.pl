eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Compare qw/compare_text/;

my $opendds_idl = PerlDDS::get_opendds_idl();
if (!defined($opendds_idl)) {
  print STDERR "ERROR: Couldn't get opendds_idl\n";
  exit 1;
}

sub gen_ts {
  my $out = shift();

  my $gencmd = "$opendds_idl  -Sa -St sample.idl -o $out";
  system($gencmd) == 0 or die "ERROR: could not run \"$gencmd\": $?\n";
}

sub compare_lines {
  my $a = shift();
  my $a_line = shift();
  my $b = shift();
  my $b_line = shift();

  # OPENDDS_IDL_FILE_SPECIFIC is defined dynamically, so has to be ignored.
  my $re = qr/define OPENDDS_IDL_FILE_SPECIFIC/;
  if ($a_line ne $b_line && !($a_line =~ $re && $b_line =~ $re)) {
    print STDERR (
      "ERROR: files differ starting on line $.\n" .
      "ERROR: This line in \"$a\": $a_line" .
      "ERROR: This line in \"$b\": $b_line");
    return 1;
  }
  return 0;
};

gen_ts('a');
gen_ts('b');
my $tsi = 'sampleTypeSupportImpl.cpp';
my $a = "a/$tsi";
my $b = "b/$tsi";
my $result = compare_text($a, $b, sub {compare_lines($a, $_[0], $b, $_[1])});
print "test " . ($result == 0 ? "PASSED" : "FAILED") . "\n";

exit $result;
