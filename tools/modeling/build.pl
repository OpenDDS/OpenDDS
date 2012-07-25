eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Starting from a fresh svn checkout, build the OpenDDS Modeling SDK feature
# and its constituent plugins.
# Without --automated, just runs the code generation steps for EMF and GMF.
#   (This is meant for use "interactively" by a developer who will then use
#    Eclipse the GUI to actually build the Java code.)
# Requires the ECLIPSE_HOME environment variable to be set.
# If ECLIPSE_WORKSPACE is set, use that workspace, otherwise use a temp dir
#   (Temp dir is currently experimental, for now make sure this is an actual
#    workspace with the tools/modeling projects loaded in it -- Eclipse does
#    not provide a scriptable "headless" way to import projects.)
# If ECLIPSE_EXTRA_PLUGINS is set, use it to locate plugins not installed in
# Eclipse's home directory (for example, plugins installed under $HOME).

use strict;
use Env qw(ECLIPSE_HOME ECLIPSE_WORKSPACE ECLIPSE_EXTRA_PLUGINS);
use Cwd;
use File::Temp qw(tempdir);
use File::Path;

my $suffix = ($^O eq 'MSWin32') ? 'c' : '';
my $cwd = getcwd();
my $feature_dir = 'features/org.opendds.modeling.feature';
my @steps = (
  {'dir' => 'plugins/org.opendds.modeling.model',
   'script' => 'ant_codegen.xml'},
  {'dir' => 'plugins/org.opendds.modeling.sdk.model',
   'script' => 'ant_codegen.xml'},
  {'dir' => 'plugins/org.opendds.modeling.gmf',
   'script' => 'ant_codegen.xml'});

if (scalar @ARGV && $ARGV[0] =~ /^-?-automated/) {
  unshift(@steps,
          {'dir' => $feature_dir,
           'args' => 'clean'}) if -r $feature_dir . '/build.xml';
  push(@steps,
       {'extra_plugins' => 1},
       {'dir' => $feature_dir});
}

if (!defined $ECLIPSE_WORKSPACE) {
  $ECLIPSE_WORKSPACE = tempdir();
}

$ENV{'LANG'} = 'en_US.UTF-8';

foreach my $s (@steps) {
  chdir $cwd . ($s->{'dir'} ? '/' . $s->{'dir'} : '');

  my @args;
  if ($s->{'script'}) {
    push(@args, "-f $s->{'script'}");
  }
  if ($s->{'extra_plugins'} && $ECLIPSE_EXTRA_PLUGINS) {
    push(@args, "-Dextra.plugins=$ECLIPSE_EXTRA_PLUGINS");
  }
  if ($s->{'args'}) {
    push(@args, $s->{'args'});
    if ($s->{'args'} eq 'clean') {     # extra 'clean' step to remove bin dirs
      rmtree([glob('../../plugins/*/bin')]);  # current dir is the $feature_dir
    }
  }

  my $cmd = "\"$ECLIPSE_HOME/eclipse$suffix\" -nosplash --launcher.suppressErrors -data " .
      "$ECLIPSE_WORKSPACE -application org.eclipse.ant.core." .
      "antRunner -l ant.log @args -vmargs -Xmx1g";
  print "$cmd\n";

  my $status = system($cmd);

  open LOG, 'ant.log' or die "ERROR: Can't open ant.log";
  while (<LOG>) {
    print;
  }
  close LOG;
  if ($status > 0) {
    print "ERROR: Eclipse antRunner invocation failed with $status\n";
    exit($status >> 8);
  }

}
