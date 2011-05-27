#
# $Id$
#

package TAO_ICHelper;

use strict;

use FileHandle;
use IDLBase;

our @ISA = qw(IDLBase);

sub get_component_name {
  return 'Erlang_Files';
}

sub get_outputexts {
  return ['\\.erl', '\\.hrl'];
}

sub get_filenames {
  my $self = shift;
  my($flags, $type, @scope) = @_;
  
  my @filenames;

  $self->parse_flags($flags);
  
  #
  # TODO TEMPORARY HACK
  #
  if ($type ne 'const' and
      $type ne 'enum' and
      $type ne 'struct' and
      $type ne 'union') {
    return @filenames;
  }

  my $name = $self->get_scoped_name(@scope);
  push @filenames, $self->get_src_file($name);

  # Structured types have a corresponding header which contain
  # a record definition:
  if ($type eq 'struct') {
    push @filenames, $self->get_include_file($name);
  }
  
  return @filenames; 
}

sub parse_flags {
  my $self = shift;
  my($flags_) = @_;

  my @flags = split /\s+/, $flags_;

  my $len = @flags;
  for (my $i = 0; $i < $len; ++$i) {
    my $flag = @flags[$i];

    # tao_ic flags we care about:
    $self->{otp} = 1                    if $flag eq '-otp';
    $self->{output_dir} = @flags[++$i]  if $flag eq '-o';
  }
}

sub get_scoped_name {
  my $self = shift;
  my(@scope) = @_;

  # Compose name with underscores
  my $name = join '_', @scope;
 
  # Strip leading underscores
  for (;;) {
    last if !/^_/;
    $name = substr $name, 0;
  }

  # Convert to lowercase and return
  return lc $name;
}

sub normalize_path {
  my($path) = @_;

  # Append directory separator if needed 
  return $path .= '/' if $path !~ /\/$/;
}

sub get_output_dir {
  my $self = shift;
  my($dirname) = @_;

  my $output_dir;
  
  if (defined $self->{output_dir}) {
    $output_dir .= normalize_path($self->{output_dir});
  }

  if (defined $dirname && defined $self->{otp}) {
    $output_dir .= normalize_path($dirname);
  }

  return $output_dir;
}

sub get_include_file {
  my $self = shift;
  my($name) = @_;

  my $dir = $self->get_output_dir('include');
  return $dir . $name . '.hrl';
}

sub get_src_file {
  my $self = shift;
  my($name) = @_;

  my $dir = $self->get_output_dir('src');
  return $dir . $name . '.erl';
}

1;
