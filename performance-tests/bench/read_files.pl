eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use autodie;

opendir my $dir, ".";
my @files = readdir $dir;
closedir $dir;

foreach (@files) {
  print "$_\n";
}
