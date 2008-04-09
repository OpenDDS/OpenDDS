# $Id$
use PerlACE::Process;

package PerlDDS::Process;

use strict;
use English;
use POSIX qw(:time_h);

our @ISA = qw(PerlACE::Process);

sub new {
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $executable = shift;
    my $arguments = shift;
    my $self = PerlACE::Process->new($executable, $arguments);
	bless($self, $class);
	return $self;
}

sub Spawn {
  my $self = shift;
  add_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  my $ret_value = $self->SUPER::Spawn();
  remove_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  return $ret_value;
}

sub SpawnWaitKill {
  my $self = shift;
  my $seconds = shift;
  add_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  my $ret_value = $self->SUPER::SpawnWaitKill($seconds);
  remove_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  return $ret_value;
}

1;
