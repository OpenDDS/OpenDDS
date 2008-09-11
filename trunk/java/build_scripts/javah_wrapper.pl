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

my $class = shift @ARGV;
$class =~ s/\.class$//;
if ($class =~ s(^classes[/\\])()) {
    @ARGV = ('-classpath classes');
}

my $file = "$class.h";
$file =~ s([/\\])(_)g;

$class =~ s([/\\])(.)g;

my $return = 0;
my $javap = `"$JAVA_HOME/bin/javap" @ARGV -private $class`;
if ($javap =~ /native\s/ || $javap =~ /\snative/) {
    my $cmd = "\"$JAVA_HOME/bin/javah\" @ARGV $class";
    $return = system($cmd) >> 8;
}

if (-r $file) {
    my $now = time;
    utime $now, $now, $file;
} else {
    open FH, ">$file";
    close FH;
}

exit $return;
