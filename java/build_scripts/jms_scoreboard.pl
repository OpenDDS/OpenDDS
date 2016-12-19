eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# 1st arg: [build|test|jboss]
# subsequent args: passed to ant directly
# Build or test the java/jms subtree with one command.
# Requires ANT_HOME, JAVA_HOME, and JBOSS_HOME to be set.

use strict;
use Env qw(ANT_HOME DDS_ROOT ACE_ROOT);
use Cwd;
use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use PerlDDS::Run_Test;

chdir $DDS_ROOT;
my $opt_d = 'java/jms';
my $operation = shift;
my @targets = (); # each element is a list-ref with [directory, ant target]

if ($operation eq 'build') {
    @targets = (['.', 'rar'], ['compat', 'ear']);
}
elsif ($operation eq 'test') {
    @targets = (['.', 'test']);
}
elsif ($operation eq 'jboss') {
    @targets = (['compat', 'jboss42x']);
}
else {
    print "ERROR: unknown operation: $operation\n";
    exit 1;
}

if (!chdir $opt_d) {
    print "ERROR: unable to chdir to \"$opt_d\"!\n";
    exit $!;
}

my $ant = $ANT_HOME . '/bin/ant';
if (!-r $ant) {
    $ant = 'ant'; # For Linux packages, ANT_HOME is in /usr/share but the ant
}                 # launching shell script is not in ANT_HOME (just use PATH).

# This seems to only impact ant on Windows, it's confused by -Dfoo=bar when
# bar also contains an =
if ($^O eq 'MSWin32') {
    map s/-D([\w.]+)=(\S+)=(\S+)/-D$1=\\"$2=$3\\"/, @ARGV;
    $ant = '"' . $ant . '"' if $ant =~ / /;
}

map {$_ = '"' . $_ . '"' if $_ =~ / /} @ARGV;

my $overall_status = 0;

for my $tgt (@targets) {
    my $cwd = getcwd();
    if ($tgt->[0] ne '.') {
        chdir($tgt->[0]) or die "ERROR: Cannot chdir to $tgt->[0]";
    }
    my $extra = ($operation eq 'test') ? '-l ant.log' : '';
    my $status;
    if ($tgt->[1] eq 'jboss42x') {
      my $PROC;
      if ($^O eq 'MSWin32') {
        $PROC = PerlDDS::create_process("$ENV{windir}\\system32\\cmd",
                                        "/c $ant @ARGV $extra " .
                                        "$tgt->[1]");
      }
      else {
        if ($ant eq 'ant') {
          # create_process requries full path to script or else it will use ./
          my @p = split /:/, $ENV{'PATH'};
          for my $entry (@p) {
            if (-r $entry . '/ant') {
              $ant = $entry . '/ant';
            }
          }
        }
        $PROC = PerlDDS::create_process($ant, "@ARGV $extra $tgt->[1]");
      }
      $status = $PROC->SpawnWaitKill(600);
    }
    else {
      $status = system("$ant @ARGV $extra $tgt->[1]");
    }
    my $testfailed = 0;
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
                    $2 =~ /^run: \d+, Failures: (\d+), Errors: (\d+),.* Time elapsed: (\d+)/;
                    my $t = $3 . 's';
                    my $r = $1 + $2;
                    $overall_status += $r;
                    if ($r == 0) {
                      s/Errors:/Errs:/;
                      # we don't want this to get parsed as an 'error' line
                    } else {
                      $testfailed = 1;
                    }
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
        # If we've had a test failure, the scoreboard will pick that up as
        # an 'error' line and we don't need an extra one for the ant invocation
        # returning non-zero.
        if ($testfailed == 0) {
            print "ERROR: ";
        }
        print "ant invocation failed with $status\n";
        exit($status >> 8);
    }
    if ($tgt->[0] ne '.') {
        chdir($cwd) or die "ERROR: Cannot chdir to $cwd";
    }
}

exit $overall_status;
