
# This module contains a few miscellaneous functions and some
# startup ARGV processing that is used by all tests.

use PerlACE::Run_Test;
use PerlDDS::Process;
use PerlDDS::ProcessFactory;
use Cwd;
use POSIX qw(strftime);
use File::Spec;

package PerlDDS;

sub orbsvcs {
  my $o = "$ENV{'TAO_ROOT'}/orbsvcs";
  my $n = -r "$o/Naming_Service/tao_cosnaming" || # using new names?
          -r "$o/Naming_Service/tao_cosnaming.exe" ||
          -r "$o/Naming_Service/Release/tao_cosnaming.exe" ||
          -r "$o/Naming_Service/Static_Debug/tao_cosnaming.exe" ||
          -r "$o/Naming_Service/Static_Release/tao_cosnaming.exe";
  return (
    'Naming_Service' => "$o/Naming_Service/" . ($n ? 'tao_cosnaming'
                                                   : 'Naming_Service'),
    'ImplRepo_Service' => "$o/ImplRepo_Service/" . ($n ? 'tao_imr_locator'
                                                       : 'ImplRepo_Service'),
    'ImR_Activator' => "$o/ImplRepo_Service/" . ($n ? 'tao_imr_activator'
                                                    : 'ImR_Activator'),
    );
}

sub formatted_time {
  my $seconds = shift;
  return strftime('%Y-%m-%d %H:%M:%S',
                  $seconds ? localtime($seconds) : localtime());
}

sub wait_kill {
  my $process = shift;
  my $wait_time = shift;
  my $desc = shift;
  my $verbose = shift;
  $verbose = 0 if !defined($verbose);

  my $ret_status = 0;
  my $start_time = formatted_time();
  if ($verbose) {
    print STDERR "$start_time: waiting $wait_time seconds for $desc before "
      . "calling kill\n";
  }
  my $result = $process->WaitKill($wait_time);
  my $time_str = formatted_time();
  if ($result != 0) {
      my $ext = ($verbose ? "" : "(started at $start_time)");
      print STDERR "$time_str: ERROR: $desc returned $result $ext\n";
      $ret_status = 1;
  } elsif ($verbose) {
    print STDERR "$time_str: shut down $desc\n";
  }
  return $ret_status;
}

sub terminate_wait_kill {
  my $process = shift;
  my $wait_time = shift;
  my $desc = shift;
  $wait_time = 10 if !defined($wait_time);
  $desc = "DCPSInfoRepo" if !defined($desc);

  my $result = $process->TerminateWaitKill($wait_time);
  my $ret_status = 0;
  my $time_str = formatted_time();
  if ($result != 0) {
      print STDERR "$time_str: ERROR: $desc returned $result\n";
      $ret_status = 1;
  }
  return $ret_status;
}

sub print_file {
  my $file = shift;

  if ($ENV{'DDS_TEST_NO_LOG_FILES'}) {
    print STDERR "<<<<<  $file  >>>>> (skipping contents)\n";
    return;
  }

  if (open FILE, "<", $file) {
    print STDERR "<<<<<  $file  >>>>>\n";
    while (my $line = <FILE>) {
      print STDERR "$line";
    }
    print STDERR "\n<<<<<  end $file  >>>>>\n\n";
    close FILE;
  }
}

sub report_errors_in_file {
  my $file = shift;
  my $errors_to_ignore = shift;
  my $verbose = shift;
  $errors_to_ignore = [] if !defined($errors_to_ignore);
  $verbose = 0 if !defined($verbose);

  my $error = 0;
  if (open FILE, "<", $file) {
      while (my $line = <FILE>) {
          my $report = ($line =~ /ERROR/);
          if ($report) {
              # determine if this is an error we want to ignore
              foreach my $to_ignore (@{$errors_to_ignore}) {
                  if ($line =~ /$to_ignore/) {
                      $report = 0;
                      last;
                  }
              }

              if ($report) {
                  $error = 1;
              }
          }

          if (!$report && $line =~ /wait_(?:messages_)?pending /) {  # REMOVE LATER
              if ($line =~ s/ERROR/ERR0R/g) {
                  $line .= "(TestFramework ignored this as a problem)";
              }
              print STDERR "$file: $line";
          }
      }
      close FILE;
  }

  return $error;
}

# load gcov helpers in case this is a coverage build
my $config = new PerlACE::ConfigList;
$PerlDDS::Coverage_Test = $config->check_config("Coverage");
$PerlDDS::SafetyProfile = $config->check_config("OPENDDS_SAFETY_PROFILE");
$PerlDDS::security = $config->check_config("OPENDDS_SECURITY");

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
      if ((!defined $config_os) || ($config_os =~ m/local|remote/i)) {
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

package PerlDDS::TestFramework;
use strict;

sub new {
  my $class = shift;
  my $self = bless {}, $class;

  $self->{processes} = {};
  $self->{flags} = {};
  $self->{status} = 0;
  $self->{log_files} = [];
  $self->{temp_files} = [];
  $self->{errors_to_ignore} = [];
  $self->{info_repo} = {};
  $self->{info_repo}->{state} = "none";
  $self->{info_repo}->{file} = "repo.ior";
  $self->{processes}->{process} = {};
  $self->{processes}->{order} = [];
  $self->{discovery} = "rtps";
  $self->{test_verbose} = 0;
  $self->{add_transport_config} = 1;
  $self->{nobits} = 0;
  $self->{add_pending_timeout} = 1;
  $self->{transport} = "";
  $self->{report_errors_in_log_file} = 1;
  $self->{dcps_debug_level} = 1;
  $self->{dcps_transport_debug_level} = 1;
  $self->{dcps_security_debug} = defined $ENV{DCPSSecurityDebug} ?
    $ENV{DCPSSecurityDebug} : "";
  $self->{dcps_security_debug_level} = defined $ENV{DCPSSecurityDebugLevel} ?
    $ENV{DCPSSecurityDebugLevel} : "";
  $self->{add_orb_log_file} = 1;
  $self->{wait_after_first_proc} = 25;
  $self->{finished} = 0;
  $self->{console_logging} = 0;

  my $index = 0;
  foreach my $arg (@ARGV) {
    next if $arg eq '';
    $arg =~ /^([^=]*)(:?=(.*))?$/;
    $self->_info("TestFramework parsing \"$arg\"\n");
    my $flag_name = $1;
    if ($flag_name eq "") {
      print STDERR "ERROR: TestFramework got \"$arg\", which is a name value" .
        " pair with an empty name.\n";
      $flag_name = "<No Name Provided>";
    }
    my $flag_value;
    if (defined($2)) {
      $flag_value = $2;
    }
    else {
      $flag_value = "<FLAG>";
    }
    $self->_info("TestFramework storing \"$flag_name\"=\"$flag_value\"\n");
    $self->{flags}->{$flag_name} = $flag_value;
    my $transport = _is_transport($arg);
    if ($transport && $self->{transport} eq "") {
      $self->{transport} = $arg;
    } elsif ($transport) {
      print STDERR "ERROR: TestFramework got transport flag=\"$arg\", but "
        . "already got another transport flag=\"$self->{transport}\".\n";
      $transport = 0;
    }

    if ($arg =~ /^rtps_disc(?:_tcp)?$/) {
      $self->{discovery} = "rtps";
    } elsif ($arg eq "--test_verbose") {
      $self->{test_verbose} = 1;
      my $left = $#ARGV - $index;
      $self->_time_info("Test starting ($left arguments remaining)\n");
    } elsif (lc($arg) eq "nobits") {
      $self->{nobits} = 1;
    } elsif (!$transport) {
      # also keep a copy to delete so we can see which parameters
      # are unused (above args are already "used")
      $self->{flags}->{unused}->{$flag_name} = $flag_value;
    }
    ++$index;
  }

  return $self;
}

sub wait_kill {
  my $self = shift;
  my $name = shift;
  my $wait_time = shift;
  my $verbose = shift;

  my $process = $self->{processes}->{process}->{$name}->{process};

  return PerlDDS::wait_kill($process, $wait_time, $name, $verbose);
}

sub default_transport {
  my $self = shift;
  my $transport = shift;
  if ($self->{transport} eq "") {
    $self->_info("TestFramework::default_transport setting transport to "
      . "\"$transport\"\n");
    $self->{transport} = $transport;
  } else {
    $self->_info("TestFramework::default_transport not setting transport to "
      . "\"$transport\", since it is already set to \"$self->{transport}\"\n");
  }
}

sub DESTROY
{
}

sub finish {
  my $self = shift;
  my $wait_to_kill = shift;
  my $first_process_to_stop = shift;
  $self->_info("TestFramework::finish finished=$self->{finished}, "
    . "status=$self->{status}\n");

  if ($self->{finished}) {
    return;
  }
  $self->{finished} = 1;

  if (defined($wait_to_kill)) {
    $self->stop_processes($wait_to_kill, $first_process_to_stop);
    if ($self->{report_errors_in_log_file} && $self->{status} == 0) {
      $self->_info("TestFramework::finish looking for ERRORs in log files."
        . " To prevent this set <TestFramework>->{report_errors_in_log_file}"
        . "=0\n");
      foreach my $file (@{$self->{log_files}}) {
        if (PerlDDS::report_errors_in_file($file, $self->{errors_to_ignore})) {
          $self->{status} = -1;
        }
      }
    }
  }
  if ($PerlDDS::SafetyProfile && $self->{console_logging} == 1) {
    foreach my $file (@{$self->{log_files}}) {
      PerlDDS::print_file($file);
    }
    if ($self->{status} == 0) {
      print STDERR _prefix() . "test PASSED.\n";
    } else {
      print STDERR _prefix() . "test FAILED.\n";
    }
  } else {
    if ($self->{status} == 0) {
      print STDERR _prefix() . "test PASSED.\n";
    } else {
      foreach my $file (@{$self->{log_files}}) {
        PerlDDS::print_file($file);
      }
      print STDERR _prefix() . "test FAILED.\n";
    }
  }

  foreach my $file (@{$self->{temp_files}}) {
    unlink $file;
  }

  return $self->{status};
}

sub flag {
  my $self = shift;
  my $flag_passed = shift;

  my $present = defined($self->{flags}->{$flag_passed});
  $self->_info("TestFramework::flag $flag_passed present=$present\n");
  if ($present) {
    if ($self->{flags}->{$flag_passed} ne "<FLAG>") {
      print STDERR "WARNING: you are treating a name-value pair as a flag, should call value_flag. \"$flag_passed=" .
        $self->{flags}->{$flag_passed} . "\"\n";
    }
    delete($self->{flags}->{unused}->{$flag_passed});
  }
  return $present;
}

sub value_flag {
  my $self = shift;
  my $flag_passed = shift;

  my $present = defined($self->{flags}->{$flag_passed});
  $self->_info("TestFramework::value_flag $flag_passed present=$present\n");
  if ($present) {
    if ($self->{flags}->{$flag_passed} eq "<FLAG>") {
      # this is indicating if a flag with a value is present, but this is just a flag
      return 0;
    }
    delete($self->{flags}->{unused}->{$flag_passed});
  }
  return $present;
}

sub get_value_flag {
  my $self = shift;
  my $flag_passed = shift;

  my $present = defined($self->{flags}->{$flag_passed});
  $self->_info("TestFramework::get_value_flag $flag_passed present=$present\n");
  if ($present) {
    if ($self->{flags}->{$flag_passed} eq "<FLAG>") {
      print STDERR "ERROR: $flag_passed does not have a value, should not call get_value_flag\n";
    }
    if (defined($self->{flags}->{unused}->{$flag_passed})) {
      print STDERR "WARNING: calling get_value_flag($flag_passed) without first verifying that "
        . "it is present with value_flag($flag_passed)\n";
      delete($self->{flags}->{unused}->{$flag_passed});
    }
  }
  else {
    print STDERR "ERROR: $flag_passed is not present, should have called value_flag before calling get_value_flag\n";
  }
  return $present;
}

sub report_unused_flags {
  my $self = shift;
  my $exit_if_unidentified = shift;
  $exit_if_unidentified = 0 if !defined($exit_if_unidentified);

  $self->_info("TestFramework::report_unused_flags\n");
  my @unused = keys(%{$self->{flags}->{unused}});
  if (scalar(@unused) == 0) {
    return;
  }

  my $list = "";
  for my $key (@unused) {
    if ($list ne "") {
      $list .= ", ";
    }
    $list .= $key;
  }
  my $indication = ($exit_if_unidentified ? "ERROR" : "WARNING");
  print STDERR "$indication: unused command line arguments: $list\n";
  if ($exit_if_unidentified) {
    $self->{status} = -1;
    exit $self->finish();
  }
}

sub unused_flags {
  my $self = shift;

  return keys(%{$self->{flags}->{unused}});
}

sub process {
  my $self = shift;
  my $name = shift;
  my $executable = shift;
  my $params = shift;
  if (defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: already created process named \"$name\"\n";
    $self->{status} = -1;
    return;
  }

  my $subdir = $PerlACE::Process::ExeSubDir;
  my $basename = File::Basename::basename ($executable);
  my $dirname = File::Basename::dirname ($executable). '/';
  my $test_executable = $dirname.$subdir.$basename;
  if (!(-e $test_executable) && !(-e "$test_executable.exe") && !(-e $executable) && !(-e "$executable.exe")) {
    print STDERR "ERROR: executable \"$executable\" does not exist; subdir: $subdir; basename: $basename ; dirname: $dirname\n";
    $self->{status} = -1;
    return;
  }

  if (defined $ENV{DCPSDebugLevel}) {
    $self->{dcps_debug_level} = $ENV{DCPSDebugLevel};
  }

  if ($params !~ /-DCPSDebugLevel / && $self->{dcps_debug_level}) {
    my $debug = " -DCPSDebugLevel $self->{dcps_debug_level}";
    if ($params !~ /-ORBVerboseLogging /) {
      $debug .= " -ORBVerboseLogging 1";
    }
    $params .= $debug;
  }

  if (defined $ENV{DCPSTransportDebugLevel}) {
    $self->{dcps_transport_debug_level} = $ENV{DCPSTransportDebugLevel};
  }

  if ($params !~ /-DCPSTransportDebugLevel / &&
      $self->{dcps_transport_debug_level}) {
    my $debug = " -DCPSTransportDebugLevel $self->{dcps_transport_debug_level}";
    $params .= $debug;
  }

  if ($params !~ /-DCPSSecurityDebug(?:Level)? /) {
    if ($self->{dcps_security_debug}) {
      $params .=  " -DCPSSecurityDebug $self->{dcps_security_debug}";
    }
    elsif ($self->{dcps_security_debug_level}) {
      $params .=  " -DCPSSecurityDebugLevel $self->{dcps_security_debug_level}";
    }
  }

  if ($self->{add_orb_log_file} && $params !~ /-ORBLogFile ([^ ]+)/) {
    my $file_name = "$name";

    # account for "blah #2"
    $file_name =~ s/ /_/g;
    $file_name =~ s/#//g;

    my $debug = " -ORBLogFile $file_name.log";
    $params .= $debug;
  }

  if ($self->{add_transport_config} &&
      $self->{transport} ne "" &&
      $params !~ /-DCPSConfigFile /) {
    $self->_info("TestFramework::process appending "
      . "\"-DCPSConfigFile <transport>.ini\" to process's parameters. Set "
      . "<TestFramework>->{add_transport_config} = 0 to prevent this.\n");
    my $ini_file = $self->_ini_file($name);
    $params .= " -DCPSConfigFile $ini_file " if $ini_file ne "";
  }

  if ($self->{nobits}) {
    my $no_bits = " -DCPSBit 0 ";
    $params .= $no_bits;
  }

  $self->{processes}->{process}->{$name}->{process} =
    $self->_create_process($executable, $params);
}

sub _getpid {
    my $process = shift;
    if ($^O eq 'MSWin32') {
        return $process->{PROCESS}->GetProcessID();
    }
    else {
        return $process->{PROCESS};
    }
}

sub setup_discovery {
  my $self = shift;
  my $params = shift;
  my $executable = shift;
  $params = "" if !defined($params);

  if ($self->{discovery} ne "info_repo" || $PerlDDS::SafetyProfile) {
    $self->_info("TestFramework::setup_discovery not creating DCPSInfoRepo "
      . "since discovery=" . $self->{discovery} . "\n");
    return;
  }

  if (!defined($executable)) {
    $executable = "$ENV{DDS_ROOT}/bin/DCPSInfoRepo";
    if (!(-e $executable) && !(-e "${executable}.exe")) {
      if (!defined($ENV{OPENDDS_INSTALL_PREFIX})) {
        print STDERR "ERROR: Couldn't find \$DDS_ROOT/bin/DCPSInfoRepo. It " .
          "needs to be built or \$OPENDDS_INSTALL_PREFIX needs to be defined " .
          "if OpenDDS is installed.\n";
        exit 1;
      }
      $executable = "$ENV{OPENDDS_INSTALL_PREFIX}/bin/DCPSInfoRepo";
    }
  }

  if ($self->{info_repo}->{state} ne "none" &&
      $self->{info_repo}->{state} ne "shutdown") {
    print STDERR "ERROR: cannot start DCPSInfoRepo from a state of " .
      $self->{info_repo}->{state} . "\n";
    $self->{status} = -1;
  }

  $self->{info_repo}->{state} = "started";

  if ($params =~ /^(?:.* )?-o ([^ ]+)/) {
    $self->{info_repo}->{file} = $1;
    $self->_info("TestFramework::setup_discovery identified ior "
      . "file=\"$1\"\n");
  } else {
    my $ior_str = " -o $self->{info_repo}->{file}";
    $self->_info("TestFramework::setup_discovery did not identify ior "
      . "file, adding \"$ior_str\" to InfoRepo's parameters.\n");
    $params .= $ior_str;
  }
  $self->_info("TestFramework::setup_discovery unlink $self->{info_repo}->{file}\n");
  unlink $self->{info_repo}->{file};

  if ($self->{nobits}) {
    my $no_bits = " -NOBITS";
    $self->_info("TestFramework::process appending \"$no_bits\" to "
      . "InfoRepo's parameters. Set <TestFramework>->{nobits} = 0 to prevent"
      . " this.\n");
    $params .= $no_bits;
  }
  if (defined($ENV{OPENDDS_RTPS_DEFAULT_D0}) &&
      ($self->{transport} eq "mcast" ||
       $self->{transport} eq "multicast" ||
       $self->{transport} eq "multicast_async")) {
    $params .= " -FederationId $ENV{OPENDDS_RTPS_DEFAULT_D0}";
  }

  $self->{info_repo}->{process} =
    $self->_create_process($executable, $params);

  print $self->{info_repo}->{process}->CommandLine() . "\n";
  $self->{info_repo}->{process}->Spawn();
  print "InfoRepo PID: " . _getpid ($self->{info_repo}->{process}) . "\n";

  $self->_info("TestFramework::setup_discovery waiting for $self->{info_repo}->{file}\n");
  if (PerlACE::waitforfile_timed($self->{info_repo}->{file}, 30) == -1) {
    print STDERR "ERROR: waiting for $executable IOR file\n";
    $self->{status} = -1;
    exit $self->finish();
  }
}

sub start_process {
  my $self = shift;
  my $name = shift;
  my $tmp_dir_flag = shift;

  if (!defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: no process with name=$name\n";
    $self->{status} = -1;
    return;
  }

  push(@{$self->{processes}->{order}}, $name);
  my $process = $self->{processes}->{process}->{$name}->{process};

  if (defined($tmp_dir_flag)) {
    my $args = $process->Arguments();
    my $path = $self->_temporary_file_path($name, 1, '');
    if ($path !~ /[\/\\]$/) {
      $path .= ($^O eq 'MSWin32') ? '\\' : '/';
    }
    $process->Arguments($args . " $tmp_dir_flag $path");
  }

  print $process->CommandLine() . "\n";
  $process->Spawn();
  print "$name PID: " . _getpid($process) . "\n";
}

sub stop_process {
  my $self = shift;
  my $timed_wait = shift;
  my $name = shift;

  if (!defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: no process with name=$name\n";
    $self->{status} = -1;
    return 0;
  }

  # remove $name from the order list
  my @order = @{$self->{processes}->{order}};
  $self->{processes}->{order} = [];
  foreach my $list_name (@order) {
    if ($list_name ne $name) {
      push(@{$self->{processes}->{order}}, $list_name);
    }
  }

  my $kill_status =
    PerlDDS::wait_kill($self->{processes}->{process}->{$name}->{process},
                       $timed_wait,
                       $name,
                       $self->{test_verbose});
  $self->{status} |= $kill_status;
  delete($self->{processes}->{process}->{$name});
  return !$kill_status;
}

sub kill_process {
  my $self = shift;
  my $timed_wait = shift;
  my $name = shift;

  if (!defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: no process with name=$name\n";
    $self->{status} = -1;
    return 0;
  }

  # remove $name from the order list
  my @order = @{$self->{processes}->{order}};
  $self->{processes}->{order} = [];
  foreach my $list_name (@order) {
    if ($list_name ne $name) {
      push(@{$self->{processes}->{order}}, $list_name);
    }
  }

  my $kill_status =
    PerlDDS::terminate_wait_kill($self->{processes}->{process}->{$name}->{process},
                                 $timed_wait,
                                 $name,
                                 $self->{test_verbose});
  $self->{status} |= $kill_status;
  delete($self->{processes}->{process}->{$name});
  return !$kill_status;
}

sub stop_processes {
  my $self = shift;
  my $timed_wait = shift;
  # if passed, this will be the first processed WaitKilled
  my $name = shift;

  if (!defined($timed_wait)) {
    print STDERR "ERROR: TestFramework::stop_processes need to provide time "
      . "to wait as first parameter passed.\n";
    $self->{status} = -1;
    return;
  }

  $self->_info("TestFramework::stop_processes\n");

  my $subsequent_wait = $self->{wait_after_first_proc};

  while (scalar(@{$self->{processes}->{order}}) > 0) {
    if (!defined($name)) {
      my @rorder = reverse(@{$self->{processes}->{order}});
      $name = $rorder[0];
    }
    $self->stop_process($timed_wait, $name);
    # make next loop
    $name = undef;
    $timed_wait = $subsequent_wait;
  }

  $self->stop_discovery($timed_wait);
}

sub stop_discovery {
  my $self = shift;
  my $timed_wait = shift;
  my $name = "DCPSInfoRepo";

  $self->_info("TestFramework::stop_discovery in $timed_wait seconds\n");

  if ($self->{discovery} ne "info_repo" || $PerlDDS::SafetyProfile) {
    $self->_info("TestFramework::stop_discovery no discovery to stop " .
        "since discovery=" . $self->{discovery} . "\n");
    return;
  }

  if (!defined($self->{info_repo}->{state}) ||
       $self->{info_repo}->{state} eq "shutdown") {
    my $state = (!defined($self->{info_repo}->{state}) ? "" : $self->{info_repo}->{state});
    print STDERR "ERROR: TestFramework::stop_discovery cannot stop $name " .
      "since its state=$state\n";
    $self->{status} = -1;
    return;
  }

  my $term_status =
      $self->{info_repo}->{process} ?
      PerlDDS::terminate_wait_kill($self->{info_repo}->{process},
                                   $timed_wait,
                                   $name,
                                   $self->{test_verbose}) : 0;
  $self->{status} |= $term_status;

  $self->_info("TestFramework::stop_discovery unlink $self->{info_repo}->{file}\n");
  unlink $self->{info_repo}->{file};

  $self->{info_repo}->{state} = "shutdown" if $term_status == 0;
}

sub ignore_error {
  my $self = shift;
  my $error_msg = shift;
  $self->_info("TestFramework::ignore_error will ignore error messages "
    . "containing \"$error_msg\"\n");
  push(@{$self->{errors_to_ignore}}, $error_msg);
}

sub enable_console_logging {
  my $self = shift;
  $self->{dcps_debug_level} = 0;
  $self->{dcps_transport_debug_level} = 0;
  $self->{console_logging} = 1;
  if (!$PerlDDS::SafetyProfile) {
      # Emulate by using a log file and printing it later.
      $self->{add_orb_log_file} = 0;
  }
}

sub add_temporary_file {
  my $self = shift;
  my $process = shift;
  my $file = shift;
  my $path = $self->_temporary_file_path($process, 0, $file);
  push(@{$self->{temp_files}}, $path);
  unlink $path;
}

sub _temporary_file_path {
  my $self = shift;
  my $name = shift;
  my $flag = shift;
  my $file = shift;

  if (!defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: no process with name=$name\n";
    $self->{status} = -1;
    return;
  }
  my $process = $self->{processes}->{process}->{$name}->{process};

  # Remote processes use TEST_ROOT or FS_ROOT.
  if (defined($ENV{TEST_ROOT}) &&
      defined($process->{TARGET}->{TEST_ROOT}) &&
      defined($process->{TARGET}->{TEST_FSROOT})) {
    my $p = Cwd::getcwd();
    $p = PerlACE::rebase_path ($p, $ENV{TEST_ROOT}, $process->{TARGET}->{TEST_ROOT});
    if ($flag) {
      $p = PerlACE::rebase_path ($p, $process->{TARGET}->{TEST_ROOT}, $process->{TARGET}->{TEST_FSROOT});
    }
    return File::Spec->catfile($p, $file);
  }

  # Local processes use TEST_ROOT.
  for (values %{$self->{processes}->{process}}) {
    my $proc = $_->{process};
    if (defined($ENV{TEST_ROOT}) &&
      defined($proc->{TARGET}->{TEST_ROOT})) {
      my $p = Cwd::getcwd();
      $p = PerlACE::rebase_path ($p, $ENV{TEST_ROOT}, $proc->{TARGET}->{TEST_ROOT});
      return File::Spec->catfile($p, $file);
    }
  }

  # No locals or remotes.
  return File::Spec->catfile(Cwd::getcwd(), $file);
}

sub _prefix {
  my $self = shift;
  my $str = "";
  if ($self->{test_verbose}) {
    my $time_str = formatted_time();
    $str = "$time_str: ";
  }
  return $str;
}

sub _track_log_files {
  my $self = shift;
  my $data = shift;
  my $process = shift;

  $self->_info("TestFramework::_track_log_files looking in \"$data\"\n");
  if ($data =~ /-ORBLogFile ([^ ]+)/) {
    my $file = $1;
    $self->_info("TestFramework::_track_log_files found file=\"$file\"\n");
    if (defined($ENV{TEST_ROOT}) && defined($process->{TARGET}->{TEST_ROOT})) {
        $file = PerlACE::rebase_path ($file, $ENV{TEST_ROOT}, $process->{TARGET}->{TEST_ROOT});
    }
    push(@{$self->{log_files}}, $file);
    unlink $file;
  }
}

sub _create_process {
  my $self = shift;
  my $executable = shift;
  my $params = shift;

  $self->_info("TestFramework::_create_process creating executable="
    . "$executable w/ params=$params\n");
  if ($params !~ /-DCPSPendingTimeout /) {
    my $flag = " -DCPSPendingTimeout 3 ";
    my $possible_would_be = ($self->{add_pending_timeout} ? "" : "would be ");
    my $prevent_or_allow = ($self->{add_pending_timeout} ? "prevent" : "allow");
    $self->_info("TestFramework::_create_process " . $possible_would_be
      . "adding \"$flag\" to $executable's parameters. To " . $prevent_or_allow
      . " this set " . "<TestFramework>->{add_pending_timeout} = "
      . ($self->{add_pending_timeout} ? "0" : "1") . "\n");
    $params .= $flag if $self->{add_pending_timeout};
  }
  my $proc = PerlDDS::create_process($executable, $params);
  $self->_track_log_files($params, $proc);
  return $proc;
}

sub _alternate_transport {
  my $transport = shift;
  if ($transport =~ s/multicast/mcast/ ||
      $transport =~ s/unicast/uni/) {
    return $transport;
  }
  return "";
}

sub _ini_file {
  my $self = shift;
  my $name = shift;
  if ($self->{transport} eq "") {
    print STDERR "ERROR: TestFramework::_ini_file should not be called if no "
      . "transport has been identified.\n";
    $self->{status} = 1;
    return "";
  }
  my $transport = $self->{transport};
  unless (-e "$transport.ini") {
    my $alternate = _alternate_transport($transport);
    if ($alternate ne "" && -e "$alternate.ini") {
      $transport = $alternate;
    } elsif (-e "${name}_${transport}.ini") {
      $transport = $name . '_' . $transport;
    } else {
      my $transports = "$transport.ini";
      if ($alternate ne "") {
        $transports .= " or $alternate.ini";
      }
      print STDERR "ERROR: TestFramework::_init_file called but $transports "
        . "do not exist.  Either provide files, or set "
        . "<TestFramework>->{add_transport_config} = 0.\n";
      return $self->finish();
    }
  }
  return "$transport.ini";
}

sub _is_transport {
  my $param = shift;
  if ($param eq "tcp" ||
      $param eq "ipv6" ||
      $param eq "udp" ||
      $param eq "mcast" ||
      $param eq "multicast" ||
      $param eq "multicast_async" ||
      $param eq "rtps" ||
      $param eq "rtps_disc" ||
      $param eq "rtps_disc_tcp" ||
      $param eq "rtps_unicast" ||
      $param eq "rtps_uni" ||
      $param eq "shmem") {
    return 1;
  }
  return 0;
}

sub _time_info {
  my $self = shift;
  my $msg = shift;
  $self->_info($msg, 1);
}

sub _info {
  my $self = shift;
  my $msg = shift;
  my $prefix = shift;
  if (defined($prefix) && $prefix) {
    $msg = _prefix() . $msg;
  }

  if ($self->{test_verbose}) {
    print STDERR "$msg";
  }
}

sub _write_tcp_ini {
}

1;
