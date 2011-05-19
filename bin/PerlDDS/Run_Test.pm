# $Id$

# This module contains a few miscellaneous functions and some
# startup ARGV processing that is used by all tests.

use PerlACE::Run_Test;
use PerlDDS::Process;
use PerlDDS::ProcessFactory;
use Cwd;

package PerlDDS;

sub orbsvcs {
  my $o = "$ENV{'TAO_ROOT'}/orbsvcs";
  my $n = -r "$o/Naming_Service/tao_cosnaming" || # using new names?
          -r "$o/Naming_Service/tao_cosnaming.exe";
  return (
    'Naming_Service' => "$o/Naming_Service/" . ($n ? 'tao_cosnaming'
                                                   : 'Naming_Service'),
    'ImplRepo_Service' => "$o/ImplRepo_Service/" . ($n ? 'tao_imr_locator'
                                                       : 'ImplRepo_Service'),
    'ImR_Activator' => "$o/ImplRepo_Service/" . ($n ? 'tao_imr_activator'
                                                    : 'ImR_Activator'),
    );
}

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

sub is_special_sub_test()
{
  return $PerlDDS::Special_Sub;
}

sub is_special_pub_test()
{
  return $PerlDDS::Special_Pub;
}

sub is_special_InfoRepo_test()
{
  return $PerlDDS::Special_InfoRepo;
}

sub is_special_other_test()
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

sub swap_path {
    my $name   = shift;
    my $new_value  = shift;
    my $orig_value  = shift;
    my $environment = $ENV{$name};
    $environment =~ s/$orig_value/$new_value/g;
    $ENV{$name} = $environment;
}

sub swap_lib_path {
    my($new_value) = shift;
    my($orig_value) = shift;

  # Set the library path supporting various platforms.
    swap_path('PATH', $new_value, $orig_value);
    swap_path('DYLD_LIBRARY_PATH', $new_value, $orig_value);
    swap_path('LD_LIBRARY_PATH', $new_value, $orig_value);
    swap_path('LIBPATH', $new_value, $orig_value);
    swap_path('SHLIB_PATH', $new_value, $orig_value);
}

sub add_lib_path {
  my($dir) = shift;

  # add the cwd to the directory if it is relative
  if(($dir =~ /$\.\//)||
     ($dir =~ /$\.\.\//)) {
      $dir = Cwd::getcwd() . "/$dir";
  }

  PerlACE::add_lib_path($dir);
}

# Add PWD to the load library path
add_lib_path ('.');

$sleeptime = 5;

1;
