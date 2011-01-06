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

my $suffix = ($^O eq 'MSWin32') ? 'c' : '';
my $cwd = getcwd();
my @steps = (
  {'dir' => 'plugins/org.opendds.modeling.model',
   'script' => 'ant_codegen.xml'},
#modeling.sdk isn't ready for automated build yet:
#  {'dir' => 'plugins/org.opendds.modeling.sdk.model',
#   'script' => 'ant_codegen.xml'},
  {'dir' => 'plugins/org.opendds.modeling.gmf',
   'script' => 'ant_codegen.xml'});

my $automated = 0;
if (scalar @ARGV && $ARGV[0] =~ /^-?-automated/) {
  $automated = 1;
  push(@steps,
       {'extra_plugins' => 1},
       {'dir' => 'features/org.opendds.modeling.feature',
#  clean doesn't work yet...        'args' => 'clean build.update.jar'
       });
}

if (!defined $ECLIPSE_WORKSPACE) {
  $ECLIPSE_WORKSPACE = tempdir();
}

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
  }

  my $cmd = "\"$ECLIPSE_HOME/eclipse$suffix\" -nosplash -data " .
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

if ($automated) {
  chdir $cwd . '/icons';
  my $status = system("$^X copy-icons-to-plugins.pl");
  if ($status > 0) {
    print "ERROR: copy-icons-to-plugins.pl invocation failed with $status\n";
    exit($status >> 8);
  }
}
