package IDL2JNIHelper;

# ************************************************************
# Description   : Assist in determining the output from idl2jni.
#                 Much of the file processing code was lifted
#                 directly from the IDL compiler for opalORB.
# Author        : Chad Elliott
# Create Date   : 7/1/2008
# ************************************************************

# ************************************************************
# Pragmas
# ************************************************************

use strict;
use FileHandle;

use CommandHelper;
use IDLBase;

our @ISA = qw(IDLBase);

# ************************************************************
# Data Section
# ************************************************************

my $helper = 'Helper';
my $holder = 'Holder';
my @local  = ('LocalBase', 'TAOPeer');
my $ops    = 'Operations';
my $stub   = 'Stub';
my $ext    = '.java';

# ************************************************************
# File Processing Subroutine Section
# ************************************************************

sub get_component_name {
  return 'java_files';
}

sub get_outputexts {
  return ['\\.java'];
}

sub get_filenames {
  my $self = shift;
  my($flags, $type, $scope, $name) = @_;

  my $bits = $self->get_type_bits($type);
  my @filenames;

  ## Get the file names based on the type of data structure
  push(@filenames, basename($scope) . $ext)           if ($bits & 0x01);
  push(@filenames, basename($scope) . $holder. $ext)  if ($bits & 0x02);
  push(@filenames, basename($scope) . $helper. $ext)  if ($bits & 0x04);
  push(@filenames, basename($scope) . $ops . $ext)    if ($bits & 0x08);
  push(@filenames, privname($scope) . $stub . $ext)   if ($bits & 0x10);
  if ($bits & 0x20) {
    foreach my $local_suffix (@local) {
      push(@filenames, privname($scope) . $local_suffix . $ext);
    }
  }
  return \@filenames;
}

sub basename {
  my($scope) = @_;
  return join '/', @$scope;
}

sub privname {
  my($scope) = @_;

  ## Create a temporary for modification
  my @tmp = @$scope;

  ## Prefix last element with underscore
  my $name = pop @tmp;
  push @tmp, '_' . $name;

  return basename(\@tmp);
}

1;
