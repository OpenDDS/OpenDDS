eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Usage: update_version.pl <old_version_X.Y.Z> <new_version_X.Y.Z>

use strict;

my @patterns = qw!features/org.opendds.modeling.feature/feature.xml
                  features/org.opendds.modeling.site/site.xml
                  plugins/*/META-INF/MANIFEST.MF
                  plugins/org.opendds.modeling.gmf/diagrams/*.gmfgen
                  plugins/org.opendds.modeling.common/about.properties!;

if ($#ARGV < 1) {
  die "Usage: update_version.pl <old_version_X.Y.Z> <new_version_X.Y.Z>\n";
}

my ($old, $new) = @ARGV;
$old = quotemeta($old);

for my $pat (@patterns) {
  for my $file (glob $pat) {
    open(IN, $file) or die "can't read $file";
    my @lines = ();
    my $match = 0;
    while (<IN>) {
      if (s/$old/$new/g) {
        $match = 1;
      }
      push(@lines, $_);
    }
    close IN;
    if ($match) {
      open(OUT, '>' . $file) or die "can't write $file";
      print OUT @lines;
      close OUT;
      print "Updated $file\n";
    }
  }
}
