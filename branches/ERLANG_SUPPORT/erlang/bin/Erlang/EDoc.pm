#
# $Id$
#

package Erlang::EDoc;

use strict;
use warnings;

sub new {
  my $self = {};
  bless $self, shift;

  $self->{application} = shift or die;
  $self->{filename} = shift or ".";
  $self->{options} = shift or {};
  
  return $self; 
}

sub generate {
  my $self = shift;
  my @options = ();

  my $cmd = "erl -noshell -run edoc_run application" .
    " \"'$self->{application}'\"" .
    " \"'$self->{filename}'\"" .
    " \"'[" . join ',', $options . "]'\"";
  #system($cmd)
  print "$cmd\n";
}

1;
