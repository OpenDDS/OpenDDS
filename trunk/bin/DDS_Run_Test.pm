# $Id$

# This module contains a few miscellanous functions and some
# startup ARGV processing that is used by all tests.

use PerlACE::Run_Test;
use Process;
use ProcessFactory;

package PerlDDS;

# load gcov helpers in case this is a coverage build
my $config = new PerlACE::ConfigList;
$PerlDDS::Coverage_Test = $config->check_config("Coverage");

$PerlDDS::Special_InfoRepo = $config->check_config("Special_InfoRepo");

$PerlDDS::Special_Sub = $config->check_config("Special_Sub");

$PerlDDS::Special_Pub = $config->check_config("Special_Pub");

$PerlDDS::Special_Other = $config->check_config("Special_Other");

# used to prevent multiple processes from running remotely
$PerlDDS::Coverage_Process_Created = 0;

sub is_coverage_test()
{
  return $PerlDDS::Coverage_Test;
}

sub is_coverage_sub_test()
{
  return $PerlDDS::Special_Sub;
}

sub is_coverage_pub_test()
{
  return $PerlDDS::Special_Pub;
}

sub is_coverage_InfoRepo_test()
{
  return $PerlDDS::Special_InfoRepo;
}

sub is_coverage_other_test()
{
  return $PerlDDS::Special_Other;
}

sub is_special_process_created()
{
  return $PerlDDS::Special_Process_Created;
}

sub special_process_created()
{
  $PerlDDS::Special_Process_Created = 1;
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
