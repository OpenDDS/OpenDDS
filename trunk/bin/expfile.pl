eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use Env qw(ACE_ROOT);
my $lib = shift;
my $file = $lib . '_export.h';
if (!-e $file || -z $file) {
  # Prefer special variable $^X for perl location, rather than relying on path
  # Within system(), PATH can be lost, or other version of perl maybe invoked.
  my $s = system("$^X $ACE_ROOT/bin/generate_export_file.pl $lib > $file");
  if ($s > 0) {
    print "ERROR: generate_export_file.pl failed with $s\n";
    exit($s >> 8);
  }
}
