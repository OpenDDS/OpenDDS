eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

#
# Calls Apache Ant in a specified directory (-d)
# (using $ANT_HOME to find it).  The -d directory
# can either be absolute or relative to $DDS_ROOT.
#

use Env qw(ANT_HOME DDS_ROOT);
use Getopt::Std;

getopts('d:');

chdir $DDS_ROOT;

if (!chdir $opt_d) {
    print "ERROR: unable to chdir to \"$opt_d\"!\n";
    exit $!;
}

my $status = system("\"$ANT_HOME/bin/ant\" @ARGV");
exit($status >> 8);
