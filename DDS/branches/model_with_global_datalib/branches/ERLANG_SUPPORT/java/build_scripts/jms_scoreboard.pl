eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# 1st arg: [build|test]
# subsequent args: passed to ant directly
# Build or test the java/jms subtree with one command.
# Requires ANT_HOME, JAVA_HOME, and JBOSS_HOME to be set.

use Env qw(ANT_HOME DDS_ROOT ACE_ROOT);
use Cwd;
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

chdir $DDS_ROOT;
my $opt_d = 'java/jms';
my $operation = shift;
my @targets = (); #each element is a list-ref with [directory, ant target]

if ($operation eq 'build') {
    @targets = (['.', 'rar'], ['compat', 'ear']);
}
elsif ($operation eq 'test') {
    @targets = (['.', 'test'], ['compat', 'jboss42x']);
}
else {
    print "ERROR: unknown operation: $operation\n";
    exit 1;
}

if (!chdir $opt_d) {
    print "ERROR: unable to chdir to \"$opt_d\"!\n";
    exit $!;
}

my $overall_status = 0;

for my $tgt (@targets) {
    my $cwd = getcwd();
    if ($tgt->[0] ne '.') {
        chdir($tgt->[0]) or die "ERROR: Cannot chdir to $tgt->[0]";
    }
    my $extra = ($operation eq 'test') ? '-l ant.log' : '';
    my $status;
    if ($tgt->[1] eq 'jboss42x') {
      my $PROC = new PerlACE::Process("$ANT_HOME/bin/ant",
                                      "@ARGV $extra $tgt->[1]");
      $status = $PROC->SpawnWaitKill(300);
    }
    else {
      $status = system("\"$ANT_HOME/bin/ant\" @ARGV $extra $tgt->[1]");
    }
    if ($operation eq 'test') {
        open LOG, 'ant.log' or die "ERROR: Can't open ant.log";
        my $testclass;
        while (<LOG>) {
            chomp;
            if (/^\s*\[junit\] (Tests|Running) (.*)/) {
                if ($1 eq 'Running') {
                    $testclass = $2;
                    print "auto_run_tests: $testclass\n$_\n";
                }
                elsif ($1 eq 'Tests') {
                    $2 =~ /^run: \d+, Failures: (\d+), Errors: (\d+), Time elapsed: (\d+)/;
                    my $t = $3 . 's';
                    my $r = $1 + $2;
                    $overall_status += $r;
                    s/Errors:/Errs:/; #we don't want this to get parsed as an
                                      #'error' line on the scoreboard
                    print "$_\n\nauto_run_tests_finished: $testclass Time:$t ".
                        "Result:$r\n";
                }
                else {
                    print "$_\n";
                }
            }
        }
        close LOG;
    }
    if ($status > 0) {
        print "ERROR: ant invocation failed with $status\n";
        exit($status >> 8);
    }
    if ($tgt->[0] ne '.') {
        chdir($cwd) or die "ERROR: Cannot chdir to $cwd";
    }
}

exit $overall_status;
