eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $kind = $ARGV[0];
if (!defined($kind) || !-d $kind) {
  print STDERR ("ERROR: pass the language binding directory\n");
  exit(1);
}

my $status = 0;
my $tsi = "$kind/testTypeSupportImpl.cpp";
open(my $fh, '<', $tsi) or die "ERROR: Could not open $tsi: $!";
while (my $line = <$fh>) {
  chomp($line);
  if ($line =~ /"([^"]*)"/) {
    if ($1 =~ /(_cxx_\w+)/) {
      print STDERR ("ERROR: $tsi:$. has $1 in string: $line\n");
      $status = 1;
    }
  }
  if ($line =~ /95,\s*99,\s*120,\s*120,\s*95/) {
    print STDERR ("ERROR: $tsi:$. has the bytes for \"_cxx_\" in it\n");
    $status = 1;
  }
}
if ($status) {
  print STDERR ("ERROR: $tsi has _cxx_ prefixes in strings. These should be unescaped properly\n");
}

my $test = new PerlDDS::TestFramework();
my $path = "$kind/keywords-$kind";
$test->process('test', $path);
$test->start_process('test');
if ($test->finish(5)) {
  $status = 1;
}
exit($status);
