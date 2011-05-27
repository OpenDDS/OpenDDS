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
use IDLBase qw(%types);

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
  return "Java_Files";
}

sub get_outputexts {
  return ["\\.java"];
}

sub get_filenames {
  my $self = shift;
  my($flags, $type, @scope) = @_;

  my $bits = $types{$type};
  my @filenames;

  ## Get the file names based on the type of data structure
  push(@filenames, basename(@scope) . $ext)           if ($bits & 0x01);
  push(@filenames, basename(@scope) . $holder. $ext)  if ($bits & 0x02);
  push(@filenames, basename(@scope) . $helper. $ext)  if ($bits & 0x04);
  push(@filenames, basename(@scope) . $ops . $ext)    if ($bits & 0x08);
  push(@filenames, privname(@scope) . $stub . $ext)   if ($bits & 0x10);
  if ($bits & 0x20) {
    foreach my $local_suffix (@local) {
      push(@filenames, privname(@scope) . $local_suffix . $ext);
    }
  }
  return @filenames;
}

sub basename {
  return join '/', @_;
}

sub privname {
  my(@scope, $name) = @_;
  return join '/', (@scope, '_' . $name);
}

1;
