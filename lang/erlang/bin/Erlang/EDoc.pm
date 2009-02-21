#
# $Id$
#

package Erlang::EDoc;

use strict;
use warnings;

use File::Find;

sub new {
  my $self = {};
  bless $self, shift;
  return $self->reset();
}

sub reset {
  my $self = shift;
  $self->command qw(erl -noshell -run edoc_run);
  return $self;
}

sub files {
  my $self = shift;
  my ($filename, @options) = @_;

  $self->push('files');

  my @filenames = ();
  File::Find::find(
    sub {
      /\.erl$/ or return;
      push @filenames, "\"$File::Find::name\"";
    }, $filename);
  
  $self->push_list(@filenames);
  $self->push_list(@options);

  $self->run();
}

sub command {
  my $self = shift;
  if (@_) { @{$self->{COMMAND}} = @_ }
  return @{$self->{COMMAND}};
}

sub push {
  my $self = shift;
  push @{$self->{COMMAND}}, @_;
}

sub push_list {
  my $self = shift;
  $self->push('[' . join(',', @_) . ']');
}

sub print {
  my $self = shift;
  print join(' ', $self->command) . "\n";
}

sub run {
  my $self = shift;
  $self->print();
  system $self->command;
}

1;
