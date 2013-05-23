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
          -r "$o/Naming_Service/tao_cosnaming.exe" ||
          -r "$o/Naming_Service/Release/tao_cosnaming.exe";
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

# used to prevent multiple special processes from running remotely
$PerlDDS::Special_Process_Created = 0;

$PerlDDS::Coverage_Count = 0;
$PerlDDS::Coverage_MAX_COUNT = 6;
$PerlDDS::Coverage_Overflow_Count = $PerlDDS::Coverage_MAX_COUNT;
$PerlDDS::Coverage_Processes = [];

# used for VxWorks
$PerlDDS::vxworks_test_target = undef;
$PerlDDS::added_lib_path = "";

sub return_coverage_process {
  my $count = shift;
  if ($count >= $PerlDDS::Coverage_Count) {
    print STDERR "return_coverage_process called with $count, but only" .
      ($PerlDDS::Coverage_Count - 1) . " processes have been created.\n";
    return;
  }
  $PerlDDS::Coverage_Processes->[$count] = 0;
}

sub next_coverage_process {
  my $next;
  for ($next = 0; $next < $PerlDDS::Coverage_MAX_COUNT; ++$next) {
    if (!$PerlDDS::Coverage_Processes->[$next]) {
      $PerlDDS::Coverage_Processes->[$next] = 1;
      return $next;
    }
  }
  ++$PerlDDS::Coverage_Overflow_Count;
  $next = $PerlDDS::Coverage_MAX_COUNT - 1;
  print STDERR "ERROR: maximum coverage processes reached, " .
    "$PerlDDS::Coverage_Overflow_Count processes active.\n";

  return $next;
}

sub is_coverage_test()
{
  return $PerlDDS::Coverage_Test;
}

sub is_special_process_created()
{
  return $PerlDDS::Special_Process_Created;
}

sub special_process_created()
{
  $PerlDDS::Special_Process_Created = 1;
}

sub get_test_target_config_name()
{
    # could refactor out of PerlACE::create_target
    my $component = shift;

    my $envname = "DOC_TEST_\U$component";
    if (!exists $ENV{$envname}) {
        # no test target config name
        return undef;
    }
    my $config_name = $ENV{$envname};
    # There's a configuration name
    $config_name = uc $config_name;
    return $config_name;
}

sub get_test_target_os()
{
    # could refactor out of PerlACE::create_target
    my $config_name = shift;

    $envname = $config_name.'_OS';
    if (!exists $ENV{$envname}) {
        print STDERR "$config_name requires an OS type in $envname\n";
        return undef;
    }
    my $config_os = $ENV{$envname};
    return $config_os;
}

sub create_test_target()
{
    # could refactor out of PerlACE::create_target
    my $config_name = shift;
    my $config_os = shift;

    my $target = undef;
    SWITCH: {
      if ($config_os =~ m/local|remote/i) {
        $target = new PerlACE::TestTarget ($config_name);
        last SWITCH;
      }
      if ($config_os =~ m/LabVIEW_RT/i) {
        require PerlACE::TestTarget_LVRT;
        $target = new PerlACE::TestTarget_LVRT ($config_name);
        last SWITCH;
      }
      if ($config_os =~ /VxWorks/i) {
        require PerlACE::TestTarget_VxWorks;
        $target = new PerlACE::TestTarget_VxWorks ($config_name);
        last SWITCH;
      }
      if ($config_os =~ /WinCE/i) {
        require PerlACE::TestTarget_WinCE;
        $target = new PerlACE::TestTarget_WinCE ($config_name);
        last SWITCH;
      }
      if ($config_os =~ /ANDROID/i) {
        require PerlACE::TestTarget_Android;
        $target = new PerlACE::TestTarget_Android ($config_name, $component);
        last SWITCH;
      }
      print STDERR "$config_os is an unknown OS type!\n";
    }
    return $target;
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
    if (($dir =~ /^\.\//) || ($dir =~ /^\.\.\//)) {
      $dir = Cwd::getcwd() . "/$dir";
    }

    PerlACE::add_lib_path($dir);

    if (defined($PerlDDS::vxworks_test_target)) {
        $PerlDDS::vxworks_test_target->AddLibPath($dir);
    }
    elsif (PerlACE::is_vxworks_test()) {
        # store added lib path for late created TestTargets
        $PerlDDS::added_lib_path .= $dir . ':';
    }
}

# Add PWD to the load library path
add_lib_path ('.');

$sleeptime = 5;

1;
