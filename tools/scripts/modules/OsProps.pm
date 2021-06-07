package OsProps;

use warnings;
use strict;

use File::Spec ();
use Storable qw(dclone);

my %default_props = (
  compilers => [],
  libpath => 'LD_LIBRARY_PATH',
  slash => '/',
  exeext => '',
  shext => '.sh',
  export => 'export',
  pathsep => ':',
  refpre => '${',
  refpost => '}',
  comment => '#',
);

my %all_oses = (
  windows => {
    compilers => ['cl'],
    libpath => 'PATH',
    aceplatform => 'win32',
    aceconfig => 'win32',
    java_platform => 'win32',
    slash => '\\',
    exeext => '.exe',
    shext => '.cmd',
    export => 'set',
    pathsep => ';',
    refpre => '%',
    refpost => '%',
    comment => '::',
  },
  linux => {
    compilers => ['g++', 'clang++'],
    aceplatform => 'linux_$NONSTDCOMP', # $NONSTDCOMP = clang
  },
  android => {
    needs_i2jrt_corba => 1,
    java_platform => 'android',
  },
  macos => {
    compilers => ['clang++'],
    aceplatform => 'macosx',
    aceconfig => 'macosx',
    java_platform => 'darwin',
  },
  ios => {
    aceconfig => 'macosx-iOS',
    aceplatform => 'macosx_iOS',
  },
  freebsd => {
    compilers => ['clang++'],
  },
  vxworks => {
  },
  solaris => {
    compilers => ['CC', 'g++'],
    aceplatform => 'sunos5_$COMP', # $COMP = sunc++ or g++
    aceconfig => 'sunos$UNAMER', # $UNAMER = `uname -r`
  },
  'lynxos-178' => {
    aceplatform => 'lynxos',
  },
);

sub set_default {
  my $os = shift;
  my $prop = shift;
  my $default = shift;

  $all_oses{$os}->{$prop} = $default unless (exists($all_oses{$os}->{$prop}));
}

for my $os (keys(%all_oses)) {
  $all_oses{$os}->{name} = $os;
  set_default($os, 'aceplatform', $os);
  set_default($os, 'aceconfig', $os);
  for my $prop (keys(%default_props)) {
    set_default($os, $prop, $default_props{$prop});
  }
}

sub this_os {
  my %table = (
    linux => 'linux',
    MSWin32 => 'windows',
    freebsd => 'freebsd',
    solaris => 'solaris',
    darwin => 'macos',
  );
  die("ERROR: Running on $^O is unsupported!") unless (exists($table{$^O}));
  return %table{$^O};
}

sub new {
  my $class = shift;
  my $args = shift;
  my $debug = $args->{debug} // 0;
  my $name = $args->{name} // this_os();
  my $kind = $args->{kind} // 'host';

  print("$kind OS is $name\n") if ($debug);
  die("OS is not supported: \"$name\"") unless (exists($all_oses{$name}));

  my $self = dclone($all_oses{$name});
  $self->{debug} = $debug;

  return bless($self, $class);
}

sub which {
  my $self = shift;
  my $file = shift;

  for my $p (File::Spec->path()) {
    next if $p eq '.';
    my $path = "$p$self->{slash}$file";
    if (-x $path) {
      return $path;
    }
    elsif ($self->{is_windows} && -x "$path$self->{exeext}") {
      return "$path$self->{exeext}";
    }
  }
  return undef;
}

sub envvar {
  my $self = shift;
  my $name = shift;

  return "$self->{refpre}$name$self->{refpost}";
}

sub path {
  my $self = shift;

  return join($self->{slash}, @_);
}

sub get_compiler {
  my $self = shift;

  my $compiler = undef;
  for my $stdcomp (@{$self->{compilers}}) {
    my $path = $self->which($stdcomp);
    if ($path) {
      print("Found $stdcomp at: $path\n") if ($self->{debug});
      $compiler = $stdcomp;
      last;
    }
  }
  return $compiler;
}

1;
