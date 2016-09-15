package PerlDDS::Process_Java;

use strict;
use Env qw(JAVA_HOME DDS_ROOT ACE_ROOT TAO_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Process;

our @ISA = qw(PerlACE::Process);
PerlACE::add_lib_path("$DDS_ROOT/lib");
PerlACE::add_lib_path("$ACE_ROOT/lib") if $^O eq 'darwin';

## Constructor Arguments:
## 1. Main Class
## 2. Arguments to main() in Java as a string
## 3. Classpath as a Perl list reference, for example ['foo.jar', 'bar.jar']
##    "classes" and the DDS libraries are already included.
## 4. VM arguments as an optional string, default is '-Xcheck:jni -ea'
sub new {
  my($class, $main, $args, $classpath, $vmargs) = @_;

  my $arguments = $vmargs ? $vmargs : '-Xcheck:jni -ea';

  $main =~ s/[\/\\]/./g;
  $main =~ s/\.class$//;

  my @classpaths = ('classes', "$DDS_ROOT/lib/i2jrt.jar",
                    "$DDS_ROOT/lib/OpenDDS_DCPS.jar");

  if (-e "$DDS_ROOT/lib/i2jrt_compact.jar") {
    push(@classpaths, "$DDS_ROOT/lib/i2jrt_compact.jar");
  }

  if (defined $classpath) {
    push(@classpaths, @$classpath);
  }
  my $sep = ':';
  my $jnid;
  if ($^O eq 'MSWin32') {
    $sep = ';';
    $jnid = ' -Dopendds.native.debug=true'
      unless $PerlACE::Process::ExeSubDir =~ /Release/i;
  }

  $arguments .= ' -cp ' . join($sep, @classpaths) . $jnid . ' '
      . $main . ' ' . $args;

  my $self = $class->SUPER::new("$JAVA_HOME/bin/java", $arguments);
  bless($self, $class);
  $self->IgnoreExeSubDir(1);
  return $self;
}

1;
