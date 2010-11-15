eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Starting from a fresh svn checkout, build the OpenDDS Modeling SDK feature
# and its constituent plugins.
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
  {'dir' => 'plugins/org.opendds.modeling.gmf',
   'script' => 'ant_codegen.xml'},
  {'extra_plugins' => 1},
  {'dir' => 'features/org.opendds.modeling.feature',
   'javac_ver' => '1.6'});

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
  if ($s->{'javac_ver'}) {
    foreach ('source', 'target') {
      push(@args, "-Dant.build.javac.$_=$s->{'javac_ver'}");
    }
  }

  my $status = system("\"$ECLIPSE_HOME/eclipse$suffix\" -nosplash -data " .
                      "$ECLIPSE_WORKSPACE -application org.eclipse.ant.core." .
                      "antRunner -l ant.log @args -vmargs -Xmx1g");

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

