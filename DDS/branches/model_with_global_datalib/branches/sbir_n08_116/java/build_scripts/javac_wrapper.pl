eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Calls the javac compiler (using $JAVA_HOME to find it).  Allows composable,
# platform-independent arguments.  Most useful is -classpath.  You can
# specify multiple -classpath arguments to this script and they will be
# combined with ; on Win32 and with : on POSIX.

use Env qw(JAVA_HOME);
use strict;

my $sep = ($^O eq 'MSWin32') ? ';' : ':';

my %added;
my @newARGV;
my $lastarg;
for my $arg (@ARGV) {
  $arg = '-classpath' if $arg eq '-cp';
  if ($arg =~ /^-\w+(path|dirs)$/) {
    $lastarg = $arg;
  } elsif ($lastarg) {
    if (exists $added{$lastarg}) {
      $added{$lastarg} .= $sep . $arg;
    } else {
      $added{$lastarg} = $arg;
    }
    undef $lastarg;
  } else {
    push @newARGV, $arg;
  }
}

for my $key (keys %added) {
  unshift @newARGV, $key, $added{$key};
}

my $status = system("\"$JAVA_HOME/bin/javac\" @newARGV");
exit($status >> 8);

