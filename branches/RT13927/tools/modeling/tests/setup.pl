eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Test setup for OpenDDS Modeling SDK tests that require code generation steps
# before they have .mpc files available for mwc.pl to see.
# This should be run after ../build.pl and before MPC.

use strict;
use Env qw(DDS_ROOT JAVA_HOME);
use Cwd;

# my @dirs = qw(Codegen Messenger MessengerSplit MessengerMixed MessengerNoPub MessengerNoSub);
my @dirs = qw(Messenger MessengerSplit MessengerMixed MessengerNoPub MessengerNoSub);

my $javapkg = 'org.opendds.modeling.sdk';
my $subdir = 'model';
my @suffixes = qw(.idl _T.h _T.cpp .mpc);
my @xsls = glob "$DDS_ROOT/tools/modeling/plugins/$javapkg/xml/*.xsl";

sub generate {
  my $base = shift;
  my $cwd = getcwd();
  print "Running code generation on: $cwd/$base.opendds\n";
  my $status = system("\"$JAVA_HOME/bin/java\" -classpath " .
                      "$DDS_ROOT/tools/modeling/plugins/$javapkg/bin " .
                      "$javapkg.codegen.CodeGenerator -o $subdir " .
                      "$base.opendds");
  if ($status > 0) {
    print "ERROR: Java CodeGenerator invocation failed with $status\n";
    exit($status >> 8);
  }
}

my $cwd = getcwd();
foreach my $dir (@dirs) {
  chdir $cwd . '/' . $dir or die "Can't change to $dir\n";
  my @ddsfiles = glob '*.opendds';
  if ($#ddsfiles == -1) {
    die "Can't find an .opendds file in " . getcwd() . "\n";
  }

  INPUT: foreach my $base (@ddsfiles) {
    #print "Considering $cwd/$dir/$base\n";
    my $mtime = (stat $base)[9];
    $base =~ s/\.opendds$//;

    my @outputs = map {"$subdir/$base$_"} @suffixes;
    my %modtimes;
    foreach my $genfile (@outputs) {
      next if -e $genfile && ($modtimes{$genfile} = (stat _)[9]) > $mtime;
      #print "\tneed to generate it because it is newer than $genfile\n";
      generate($base);
      next INPUT;
    }

    foreach my $xsl (@xsls) {
      foreach my $genfile (@outputs) {
        my $mod = $modtimes{$genfile};
        if (!defined $mod) {
          $mod = $modtimes{$genfile} = (stat $genfile)[9];
        }
        if ((stat $xsl)[9] > $mod) {
          #print "\t$xsl is newer than $genfile\n";
          generate($base);
          next INPUT;
        }
      }
    }
  }
}
