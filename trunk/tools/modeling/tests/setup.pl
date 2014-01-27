eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Test setup for OpenDDS Modeling SDK tests that require code generation steps
# before they have .mpc files available for mwc.pl to see.
# This should be run after ../build.pl and before MPC.

use strict;
use Cwd;
use Env qw(ACE_ROOT DDS_ROOT JAVA_HOME);
use lib "$ACE_ROOT/bin", "$DDS_ROOT/bin";
use PerlDDS::Run_Test;

my $dir = 'tools/modeling/tests';
my $test_lst = "$DDS_ROOT/$dir/modeling_tests.lst";

my $force = 0;
if ($#ARGV >= 0 && $ARGV[0] eq '-f') {
    print "Setting force continue for failed tests\n";
    $force = 1;
    shift @ARGV;
}

my $cwd = getcwd();
if (defined $ENV{'ANT_HOME'}) {
  chdir '../plugins/org.opendds.modeling.validation';
  my $ant = $ENV{'ANT_HOME'} . '/bin/ant';
  if (!-r $ant) {
    $ant = 'ant';
  }
  my $status = system("\"$ant\" -f build-external.xml run.tests");
  if ($status > 0) {
    print "ERROR: ant invocation failed with $status\n";
    die "ant run.tests target failed and forced is off. Exiting...\n" if $force == 0;
  }
  chdir $cwd;
}
else {
  print "Warning: skipping model validation due to lack of ANT_HOME\n";
}


sub get_dirs {
  if ($#ARGV >= 0) {
    print "Overriding dir list\n";
    return @ARGV;
  }

  my $config_list = new PerlACE::ConfigList;
  $config_list->load($test_lst);
  $config_list->add_one_config('COMPILE_ONLY');
  return map {s/^$dir\/(.*)\/[^\/]*$/$1/; $_} $config_list->valid_entries();
}

my $javapkg = 'org.opendds.modeling.sdk';
my $plugin = 'org.opendds.modeling.sdk.model.editor';

sub generate {
  my $base = shift;
  my $status;

  my $plugdir = "$DDS_ROOT/tools/modeling/plugins/$plugin";
  my $jclass = "$javapkg.model.GeneratorSpecification.Generator.SdkGenerator";
  my $classfile = $jclass;
  $classfile =~ s!\.!/!g;
  $classfile .= '.class';
  if (! -r "$plugdir/bin/$classfile") {
    print "Compiling Java SdkGenerator\n";
    my $cwd = getcwd();
    chdir "$plugdir/src";
    $classfile =~ s/\.class$/.java/;
    mkdir '../bin' unless -d '../bin';
    $status = system("\"$JAVA_HOME/bin/javac\" -g -d ../bin $classfile");
    if ($status > 0) {
      print "ERROR: Java compiler invocation failed with $status\n";
      exit($status >> 8);
    }
    chdir $cwd;
  }

  print "Running code generation on: $base\n";
  $status = system("\"$JAVA_HOME/bin/java\" -classpath $plugdir/bin " .
                   "$jclass $base");
  if ($status > 0) {
    print "ERROR: Java SdkGenerator invocation failed with $status\n";
    exit($status >> 8);
  }
}

open MWC, '>modeling_tests.mwc' or die "Can't write modeling_tests.mwc";
print MWC "workspace {\n";

foreach my $dir (get_dirs()) {
  my $old_sep = $/;
  $/ = '/';
  chomp $dir;
  $/ = $old_sep;

  chdir $cwd . '/' . $dir or die "Can't change to $dir\n";
  my @ddsfiles = glob '*.codegen';
  if ($#ddsfiles == -1) {
    print "WARN Can't find a .codegen file in " . getcwd() . "\n";
  }

  foreach my $base (@ddsfiles) {
    #print "Considering $cwd/$dir/$base\n";
    generate($base);
  }

  foreach my $mwc (glob '*.mwc') {
    print MWC '  ' . $dir . "/$mwc\n";
  }
}

print MWC "}\n";
close MWC;
