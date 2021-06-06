package OsProps;

use warnings;
use strict;

use File::Spec ();

sub new {
  my $class = shift;
  my $args = shift;
  my $is_windows = $args->{is_windows} // $^O eq 'MSWin32';

  my $self = $is_windows ? {
    'is_windows' => 1,
    'slash' => '\\',
    'exeext' => '.exe',
    'shext' => '.cmd',
    'export' => 'set',
    'pathsep' => ';',
    'refpre' => '%',
    'refpost' => '%',
    'comment' => '::',
  } : {
    'is_windows' => 0,
    'slash' => '/',
    'exeext' => '',
    'shext' => '.sh',
    'export' => 'export',
    'pathsep' => ':',
    'refpre' => '${',
    'refpost' => '}',
    'comment' => '#',
  };

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

1;
