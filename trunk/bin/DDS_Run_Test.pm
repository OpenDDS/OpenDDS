# $Id$

# This module contains a few miscellanous functions and some
# startup ARGV processing that is used by all tests.

use PerlACE::Run_Test;
use Process;
use ProcessFactory;

package PerlDDS;

# load gcov helpers in case this is a coverage build
$PerlDDS::Coverage_Test = $PerlACE::TestConfig->check_config("Coverage");

$PerlDDS::Remote_InfoRepo = $PerlACE::TestConfig->check_config("Remote_InfoRepo");

$PerlDDS::Remote_Sub = $PerlACE::TestConfig->check_config("Remote_Sub");

$PerlDDS::Remote_Pub = $PerlACE::TestConfig->check_config("Remote_Pub");

$PerlDDS::Remote_Other = $PerlACE::TestConfig->check_config("Remote_Other");

# used to prevent multiple processes from running remotely
$PerlDDS::Remote_Process_Created = 0;

sub is_coverage_test()
{
  return $PerlDDS::Coverage_Test;
}

sub is_remote_sub_test()
{
  return $PerlDDS::Remote_Sub;
}

sub is_remote_pub_test()
{
  return $PerlDDS::Remote_pub;
}

sub is_remote_InfoRepo_test()
{
  return $PerlDDS::Remote_InfoRepo;
}

sub is_remote_other_test()
{
  return $PerlDDS::Remote_other;
}

sub is_remote_process_created()
{
  return $PerlDDS::Remote_Process_Created;
}

sub remote_process_created()
{
  $PerlDDS::Remote_Process_Created = 1;
}

sub remove_path {
  my $name   = shift;
  my $value  = shift;
  my $separator = ($^O eq 'MSWin32' ? ';' : ':');
  if (defined $ENV{$name}) {
    if(($ENV{$name} != s/$separator$value//) &&
       ($ENV{$name} != s/^$value$separator//) &&
       ($ENV{$name} != s/^$value$//))
    {
	  print STDERR "remove_path failed!\n";
    }
  }
}

sub remove_lib_path {
  my($value) = shift;

  # Set the library path supporting various platforms.
  remove_path('PATH', $value);
  remove_path('LD_LIBRARY_PATH', $value);
  remove_path('LIBPATH', $value);
  remove_path('SHLIB_PATH', $value);

}

$sleeptime = 5;

1;
