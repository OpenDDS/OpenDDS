eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# This program is meant to be called directly after compiling a Java source
# file with javac.  It will call javah on the generated class if and only if
# the class has native methods.  The first argument to this program must be the
# relative path and filename of the Java class file.
# The remaining arguments are passed unchanged to javah.

use Env qw(JAVA_HOME);

if ($#ARGV < 0 || $ARGV[0] =~ /\.java$/) {
    print STDERR "ERROR: The first argument must be the Java class file ".
        "without \".class\"\n";
    exit 1;
}
my $sep = ($^O eq 'MSWin32') ? ';' : ':';
my @classpath;

my $prevarg;
my $i = 0;
for my $arg (@ARGV) {
  if ($prevarg eq '-classpath') {
    push(@classpath, $arg);
	# remove two args
	splice(@ARGV, $i - 1, 2);
  }
  if ($arg eq '-cp' || $arg eq '-classpath') {
    $prevarg = '-classpath';
  }
  $i += 1;
}
undef $prevarg;

my $class = shift @ARGV;
$class =~ s/\.class$//;
if ($class =~ s(^classes[/\\])()) {
    push(@classpath, 'classes');
}

if (scalar(@classpath)) {
  $classpath = join($sep, @classpath);
  push(@ARGV, '-classpath', $classpath);
}

my $file = "$class.h";
$file =~ s([/\\])(_)g;

$class =~ s([/\\])(.)g;

my $return = 0;
my $javap = `"$JAVA_HOME/bin/javap" @ARGV -private $class`;
if ($javap =~ /native\s/ || $javap =~ /\snative/) {
    my $cmd = "\"$JAVA_HOME/bin/javah\" @ARGV $class";
    $return = system($cmd) >> 8;
    if ($return == 0) {
        #Need to add our own include first in the file
        open FH, "$file" or die "Can't open $file for reading.";
        my @lines = <FH>;
        close FH;
        open FH, ">$file" or die "Can't open $file for writing.";
        for (@lines) {
            if (/\#include <jni\.h>/) {
                print FH "#include \"idl2jni_jni.h\"\n";
            } else {
                print FH;
            }
        }
        close FH;
    }
}

if (-r $file) {
    my $now = time;
    utime $now, $now, $file;
} else {
    open FH, ">$file";
    close FH;
}

exit $return;
