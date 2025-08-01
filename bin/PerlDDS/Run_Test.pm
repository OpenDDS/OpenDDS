# This module contains a few miscellaneous functions and some
# startup ARGV processing that is used by all tests.

use strict;
use warnings;

package PerlDDS;

# NOTE: There is another package called TestFramework in this file. If you need
# imports for that package, add them there, not here.

use PerlACE::Run_Test;
use PerlDDS::Process;
use PerlDDS::ProcessFactory;
use Cwd;
use POSIX qw(strftime);
use File::Spec::Functions qw(catfile catdir);

# FindBin doesn't seem to work correctly here for some reason.
BEGIN {
  unshift(@INC, catfile(File::Basename::dirname(Cwd::realpath(__FILE__)), '..', '..', 'tools', 'scripts', 'modules'))
};

use misc_utils qw/trace/;

sub is_executable {
  my $path = shift;

  return -x $path || -x "$path.exe";
}

sub get_executable {
  my $name = shift;

  for my $dir (@_) {
    next unless length($dir);
    my $path = catfile($dir, $name);
    return $path if (is_executable($path));
  }
  return undef;
}

sub get_bin_executable {
  my $name = shift;

  my $bin = catdir($ENV{DDS_ROOT}, "bin");
  my $from = "DDS_ROOT";
  if (defined($ENV{OPENDDS_BUILD_DIR})) {
    $bin = catdir($ENV{OPENDDS_BUILD_DIR}, "bin");
    $from = "OPENDDS_BUILD_DIR";
    if (defined($ENV{CMAKE_CONFIG_TYPE})) {
      my $subdir_bin = catdir($bin, $ENV{CMAKE_CONFIG_TYPE});
      if (-d $subdir_bin) {
        $bin = $subdir_bin;
      }
    }
  }
  elsif (defined($ENV{OPENDDS_INSTALL_PREFIX})) {
    $bin = catdir($ENV{OPENDDS_INSTALL_PREFIX}, "bin");
    $from = "OPENDDS_INSTALL_PREFIX";
  }
  if (!-d $bin) {
    printf STDERR "ERROR: $bin (from $from) does not exist!\n";
  }
  return get_executable($name, $bin);
}

sub get_opendds_idl {
  my $path = get_bin_executable("opendds_idl");
  if (!defined($path)) {
    my $user_macros = catfile($ENV{DDS_ROOT}, "user_macros.GNU");
    if (-r $user_macros) {
      open(my $fd, $user_macros) or trace("Could not open $user_macros: $!");
      while (my $line = <$fd>) {
        if ($line =~ /^OPENDDS_IDL\s*=\s*(.*)$/) {
          if (is_executable($1)) {
            $path = $1;
          }
          else {
            printf STDERR "ERROR: Value of OPENDDS_IDL in user_macros.GNU is invalid: $1\n";
          }
        }
      }
      close($fd);
    }
  }
  if (!defined($path)) {
    printf STDERR "ERROR: Couldn't determine where opendds_idl is\n";
  }
  return $path;
}

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
  my $name = shift;
  my $verbose = shift;
  $verbose = 0 if !defined($verbose);
  my $opts = shift;

  my $ret_status = 0;
  my $start_time = formatted_time();
  if ($verbose) {
    print STDERR "$start_time: waiting $wait_time seconds for $name before "
      . "calling kill\n";
  }

  my $result = $process->WaitKill($wait_time, $opts);

  my $time_str = formatted_time();
  if ($result != 0) {
    my $ext = ($verbose ? "(started waiting for termination at $start_time)" : "");
    if ($result == -1) {
      print STDERR "$time_str: ERROR: $name timedout $ext\n";
    }
    elsif ($result == 255) {
      print STDERR "$time_str: ERROR: $name terminated with a signal $ext\n";
    }
    else {
      print STDERR "$time_str: ERROR: $name finished and returned $result $ext\n";
    }
    $ret_status = 1;
  }
  elsif ($verbose) {
    print STDERR "$time_str: shut down $name\n";
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

    my $envname = "${config_name}_OS";
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
        $target = new PerlACE::TestTarget_Android ($config_name, "");
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

package PerlDDS::TestFramework;

use File::Spec::Functions qw(catfile catdir);
use File::Path qw(rmtree);

use misc_utils qw/trace parse_func_opts/;

use constant {
  success => 0,
  init_error => 1,
  finish_error => 1,
  dcs_timeout => 2,
  process_error => 4,
  errors_in_log => 8,
};

sub status_string {
  my $self = shift();

  if ($self->{status} == success) {
    return 'success';
  }

  my @statuses = ();
  push(@statuses, 'init_error') if ($self->{status} & init_error);
  push(@statuses, 'finish_error') if ($self->{status} & finish_error);
  push(@statuses, 'dcs_timeout') if ($self->{status} & dcs_timeout);
  push(@statuses, 'process_error') if ($self->{status} & process_error);
  push(@statuses, 'errors_in_log') if ($self->{status} & errors_in_log);
  return join(' ', @statuses);
}

sub new {
  my $class = shift;
  my $self = bless {}, $class;

  $self->{processes} = {};
  $self->{_flags} = {};
  $self->{status} = success;
  $self->{log_files} = [];
  $self->{temp_files} = [];
  $self->{errors_to_ignore} = [];
  $self->{info_repo} = {};
  $self->{info_repo}->{state} = "none";
  $self->{info_repo}->{file} = "repo.ior";
  $self->{processes}->{process} = {};
  $self->{processes}->{order} = [];
  $self->{discovery} = "info_repo";
  $self->{test_verbose} = 0;
  $self->{add_transport_config} = 1;
  $self->{nobits} = 0;
  $self->{add_pending_timeout} = 1;
  $self->{transport} = "";
  $self->{ini} = "";
  $self->{report_errors_in_log_file} = 1;
  $self->{dcps_log_level} = $ENV{DCPSLogLevel} // "";
  $self->{dcps_debug_level} = 1;
  $self->{dcps_transport_debug_level} = 1;
  $self->{dcps_security_debug} = $ENV{DCPSSecurityDebug} // "";
  $self->{dcps_security_debug_level} = $ENV{DCPSSecurityDebugLevel} // "";
  $self->{add_orb_log_file} = 1;
  $self->{wait_after_first_proc} = 25;
  $self->{finished} = 0;
  $self->{console_logging} = 0;
  $self->{dcs} = 'DCS';
  $self->{remove_dcs_after} = 1;

  my $index = 0;
  foreach my $arg (@ARGV) {
    next if $arg eq '';
    $arg =~ /^([^=]*)(:?=(.*))?$/;
    $self->_info("TestFramework parsing \"$arg\"\n");
    my $flag_name = $1;
    if ($flag_name eq "") {
      trace("TestFramework got \"$arg\", which is a name value" .
        " pair with an empty name.");
    }
    my $flag_value = $3;
    my $flag_value_str = defined($flag_value) ? "\"$flag_value\"" : 'undef';
    $self->_info("TestFramework storing \"$flag_name\"=$flag_value_str\n");
    $self->{_flags}->{$flag_name} = $flag_value;
    my $transport = _is_transport($arg);
    if ($transport && $self->{transport} eq "") {
      $self->{transport} = $arg;
    }
    elsif ($transport) {
      trace("TestFramework got transport flag=\"$arg\", but "
        . "already got another transport flag=\"$self->{transport}\".");
    }

    if ($arg =~ /^rtps_disc(?:_tcp)?$/) {
      $self->{discovery} = "rtps";
    }
    elsif ($arg eq "--test_verbose") {
      $self->{test_verbose} = 1;
      my $left = $#ARGV - $index;
      $self->_info("Test starting ($left arguments remaining)\n");
    }
    elsif (lc($arg) eq "nobits") {
      $self->{nobits} = 1;
    }
    elsif ($flag_name eq "ini" && defined($flag_value)) {
      $flag_value =~ /^([^_]+)_([^_]+).ini/;
      if ($1 eq "inforepo") {
        $self->{discovery} = "info_repo";
      }
      else {
        $self->{discovery} = $1;
      }
      $self->{transport} = $2;
      $self->{ini} = $flag_value;
    }
    elsif (exists($self->{$flag_name}) && defined($flag_value)) {
      $self->{$flag_name} = $flag_value;
    }
    elsif (!$transport) {
      # also keep a copy to delete so we can see which parameters
      # are unused (above args are already "used")
      $self->{_flags}->{unused}->{$flag_name} = $flag_value;
    }
    ++$index;
  }

  $self->remove_dcs();

  return $self;
}

sub remove_dcs {
  my $self = shift();

  rmtree($self->{dcs});
}

sub wait_for {
  my $self = shift;
  my $posting_actor = shift;
  my $condition = shift;
  my $opts = parse_func_opts({
      max_wait => 300,
  }, @_);

  my $info = "TestFramework::wait_for: \"$posting_actor\" \"$condition\"";
  $self->_info("$info\n");

  my $path = catdir($self->{dcs}, $posting_actor, $condition);
  my $start = time();
  for (my $i = 0; $i < $opts->{max_wait}; $i = $i + 1) {
    if (-e $path) {
      my $elapsed = time() - $start;
      $self->_info("$info done after $elapsed secs\n");
      return $elapsed;
    }
    sleep(1);
  }
  $self->{status} |= dcs_timeout;
  my $elapsed = time() - $start;
  trace("$info timeout after $elapsed secs\n");
}

sub wait_kill {
  my $self = shift;
  my $name = shift;
  my $wait_time = shift;
  my $verbose = shift;
  my $opts = shift;

  my $process = $self->{processes}->{process}->{$name}->{process};

  return PerlDDS::wait_kill($process, $wait_time, $name, $verbose, $opts);
}

sub default_transport {
  my $self = shift;
  my $transport = shift;
  if ($self->{transport} eq "") {
    $self->_info("TestFramework::default_transport setting transport to "
      . "\"$transport\"\n");
    $self->{transport} = $transport;
  }
  else {
    $self->_info("TestFramework::default_transport not setting transport to "
      . "\"$transport\", since it is already set to \"$self->{transport}\"\n");
  }
}

sub DESTROY {
  my $self = shift();

  $self->finish();
}

sub print_log_files {
  my $self = shift();

  foreach my $file (@{$self->{log_files}}) {
    PerlDDS::print_file($file);
  }
}

sub finish {
  my $self = shift;
  my $wait_to_kill = shift;
  my $first_process_to_stop = shift;

  my $status_str = $self->status_string();
  $self->_info("TestFramework::finish finished=$self->{finished}, "
    . "status=$self->{status} ($status_str)\n");

  if ($self->{finished}) {
    return $self->{status};
  }
  $self->{finished} = 1;

  if (defined($wait_to_kill)) {
    $self->stop_processes($wait_to_kill, $first_process_to_stop);
    if ($self->{report_errors_in_log_file} && $self->{status} == success) {
      $self->_info("TestFramework::finish looking for ERRORs in log files."
        . " To prevent this set <TestFramework>->{report_errors_in_log_file}"
        . "=0\n");
      foreach my $file (@{$self->{log_files}}) {
        if (PerlDDS::report_errors_in_file($file, $self->{errors_to_ignore})) {
          $self->{status} |= errors_in_log;
        }
      }
    }
  }

  my $sf_logging = $PerlDDS::SafetyProfile && $self->{console_logging};
  if ($sf_logging) {
    $self->print_log_files();
  }
  if ($self->{status} == success) {
    print STDERR $self->_log_prefix() . "test PASSED.\n";
  }
  else {
    if (!$sf_logging) {
      $self->print_log_files();
    }
    print STDERR $self->_log_prefix() . "test FAILED ($status_str).\n";
  }

  foreach my $file (@{$self->{temp_files}}) {
    unlink $file;
  }

  if ($self->{remove_dcs_after}) {
    $self->remove_dcs();
  }

  return $self->{status};
}

sub flag {
  my $self = shift;
  my $flag_name = shift;
  my $flag_value_ref = shift;

  my $present = exists($self->{_flags}->{$flag_name});
  $self->_info("TestFramework::flag $flag_name present=$present\n");
  if ($present) {
    my $flag_value = $self->{_flags}->{$flag_name};
    if (defined($flag_value_ref)) {
      trace("Flag \"$flag_name\" is missing required value!") if (!defined($flag_value));
      ${$flag_value_ref} = $flag_value;
    }
    else {
      trace("Flag \"$flag_name\" was passed a value, but it isn't used") if (defined($flag_value));
    }
    delete($self->{_flags}->{unused}->{$flag_name});
  }
  return $present;
}

sub report_unused_flags {
  my $self = shift;
  my $exit_if_unidentified = shift;
  $exit_if_unidentified = 0 if !defined($exit_if_unidentified);

  $self->_info("TestFramework::report_unused_flags\n");
  my @unused = keys(%{$self->{_flags}->{unused}});
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
    $self->{status} |= init_error;
    exit $self->finish();
  }
}

sub unused_flags {
  my $self = shift;

  return keys(%{$self->{_flags}->{unused}});
}

sub _process_common {
  my $self = shift;
  my $name = shift;
  my $params = shift;
  my $debug_logging = 1;

  if ($$params !~ /-DCPSLogLevel / && $self->{dcps_log_level}) {
    $$params .= " -DCPSLogLevel $self->{dcps_log_level}";
    $debug_logging = $self->{dcps_log_level} eq "debug";
  }

  if ($debug_logging) {
    if (defined $ENV{DCPSDebugLevel}) {
      $self->{dcps_debug_level} = $ENV{DCPSDebugLevel};
    }
    if ($$params !~ /-DCPSDebugLevel / && $self->{dcps_debug_level}) {
      my $debug = " -DCPSDebugLevel $self->{dcps_debug_level}";
      if ($$params !~ /-ORBVerboseLogging /) {
        $debug .= " -ORBVerboseLogging 1";
      }
      $$params .= $debug;
    }

    if (defined $ENV{DCPSTransportDebugLevel}) {
      $self->{dcps_transport_debug_level} = $ENV{DCPSTransportDebugLevel};
    }
    if ($$params !~ /-DCPSTransportDebugLevel / &&
        $self->{dcps_transport_debug_level}) {
      $$params .= " -DCPSTransportDebugLevel $self->{dcps_transport_debug_level}";
    }

    if ($$params !~ /-DCPSSecurityDebug(?:Level)? /) {
      if ($self->{dcps_security_debug}) {
        $$params .=  " -DCPSSecurityDebug $self->{dcps_security_debug}";
      }
      elsif ($self->{dcps_security_debug_level}) {
        $$params .=  " -DCPSSecurityDebugLevel $self->{dcps_security_debug_level}";
      }
    }
  }

  if ($self->{add_orb_log_file} && $$params !~ /-ORBLogFile ([^ ]+)/) {
    my $file_name = "$name";

    # account for "blah #2"
    $file_name =~ s/ /_/g;
    $file_name =~ s/#//g;

    $$params .= " -ORBLogFile $file_name.log";
  }

  if ($self->{add_transport_config} &&
      $self->{transport} ne "" &&
      $$params !~ /-DCPSConfigFile /) {
    $self->_info("TestFramework::process appending "
      . "\"-DCPSConfigFile <transport>.ini\" to process's parameters. Set "
      . "<TestFramework>->{add_transport_config} = 0 to prevent this.\n");
    my $ini_file = $self->_ini_file($name);
    $$params .= " -DCPSConfigFile $ini_file " if $ini_file ne "";
  }

  if ($self->{nobits}) {
    my $no_bits = " -DCPSBit 0 ";
    $$params .= $no_bits;
  }
}

sub process {
  my $self = shift;
  my $name = shift;
  my $executable = shift;
  my $params = shift // "";
  if (defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: already created process named \"$name\"\n";
    $self->{status} |= process_error;
    return;
  }

  my $subdir = $PerlACE::Process::ExeSubDir;
  if (defined($ENV{CMAKE_CONFIG_TYPE})) {
    $subdir = $ENV{CMAKE_CONFIG_TYPE} . (($^O eq 'MSWin32') ? '\\' : '/');
    $PerlACE::Process::ExeSubDir = $subdir;
  }

  my $basename = File::Basename::basename($executable);
  my $dirname = File::Basename::dirname($executable);
  if (!defined(PerlDDS::get_executable($basename, $dirname, catdir($dirname, $subdir)))) {
    print STDERR "ERROR: executable \"$executable\" does not exist; subdir: $subdir; basename: $basename ; dirname: $dirname\n";
    $self->{status} |= process_error;
    return;
  }

  $self->_process_common($name, \$params);

  $self->{processes}->{process}->{$name}->{process} =
    $self->_create_process($executable, $params);
}

sub java_process {
  my $self = shift;
  my $name = shift;
  my $main_class = shift;
  my $params = shift;
  my $jars = shift;
  my $vmargs = shift;

  if (defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: already created process named \"$name\"\n";
    $self->{status} |= process_error;
    return;
  }

  $self->_process_common($name, \$params);

  $self->{processes}->{process}->{$name}->{process} =
    $self->_create_java_process($main_class, $params, $jars, $vmargs);
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
    $executable = PerlDDS::get_bin_executable("DCPSInfoRepo");
    if (!defined($executable)) {
      print STDERR "ERROR: Couldn't find \$DDS_ROOT/bin/DCPSInfoRepo. It " .
        "needs to be built or \$OPENDDS_INSTALL_PREFIX needs to be defined " .
        "if OpenDDS is installed.\n";
      exit 1;
    }
  }

  if ($self->{info_repo}->{state} ne "none" &&
      $self->{info_repo}->{state} ne "shutdown") {
    print STDERR "ERROR: cannot start DCPSInfoRepo from a state of " .
      $self->{info_repo}->{state} . "\n";
    $self->{status} |= init_error;
  }

  $self->{info_repo}->{state} = "started";

  if ($params =~ /^(?:.* )?-o ([^ ]+)/) {
    $self->{info_repo}->{file} = $1;
    $self->_info("TestFramework::setup_discovery identified ior "
      . "file=\"$1\"\n");
  }
  else {
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
    $self->{status} |= init_error;
    exit $self->finish();
  }
}

sub start_process {
  my $self = shift;
  my $name = shift;
  my $tmp_dir_flag = shift;

  if (!defined($self->{processes}->{process}->{$name})) {
    print STDERR "ERROR: no process with name=$name\n";
    $self->{status} |= process_error;
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
  my $start_time = PerlDDS::formatted_time();
  print "$name PID: " . _getpid($process) . " started at $start_time\n";
}

sub _remove_process_common {
  my $self = shift();
  my $name = shift();

  my $process_info = $self->{processes}->{process}->{$name};
  if (!defined($process_info)) {
    $self->{status} |= process_error;
    trace("no process with name=$name");
  }

  # remove $name from the order list
  my @order = @{$self->{processes}->{order}};
  $self->{processes}->{order} = [];
  foreach my $list_name (@order) {
    if ($list_name ne $name) {
      push(@{$self->{processes}->{order}}, $list_name);
    }
  }

  return $process_info;
}

sub stop_process {
  my $self = shift;
  my $timed_wait = shift;
  my $name = shift;
  my $opts = shift;

  my $status = 0;
  my $process_info = $self->_remove_process_common($name);
  my $process = $process_info->{process};
  if (defined($process)) {
    if (PerlDDS::wait_kill($process, $timed_wait, $name, $self->{test_verbose}, $opts)) {
      $self->{status} |= process_error;
    }
  }
  else {
    print STDERR "process with name=$name already stopped\n";
  }
  delete($self->{processes}->{process}->{$name});
  return !$status;
}

sub kill_process {
  my $self = shift;
  my $timed_wait = shift;
  my $name = shift;

  my $process_info = $self->_remove_process_common($name);
  my $kill_status =
    PerlDDS::terminate_wait_kill($process_info->{process},
                                 $timed_wait,
                                 $name,
                                 $self->{test_verbose});
  if ($kill_status) {
    $self->{status} |= process_error;
  }
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
    $self->{status} |= finish_error;
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
    $self->{status} |= finish_error;
    return;
  }

  my $term_status =
      $self->{info_repo}->{process} ?
      PerlDDS::terminate_wait_kill($self->{info_repo}->{process},
                                   $timed_wait,
                                   $name,
                                   $self->{test_verbose}) : 0;
  if ($term_status) {
    $self->{status} |= finish_error;
  }

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
    $self->{status} |= process_error;
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
    return catfile($p, $file);
  }

  # Local processes use TEST_ROOT.
  for (values %{$self->{processes}->{process}}) {
    my $proc = $_->{process};
    if (defined($ENV{TEST_ROOT}) &&
      defined($proc->{TARGET}->{TEST_ROOT})) {
      my $p = Cwd::getcwd();
      $p = PerlACE::rebase_path ($p, $ENV{TEST_ROOT}, $proc->{TARGET}->{TEST_ROOT});
      return catfile($p, $file);
    }
  }

  # No locals or remotes.
  return catfile(Cwd::getcwd(), $file);
}

sub _log_prefix {
  my $self = shift();
  my $kind = shift();

  my $prefix = defined($kind) ? "$kind: " : '';
  if ($self->{test_verbose}) {
    my $time_str = PerlDDS::formatted_time();
    $prefix = "$time_str: $prefix";
  }
  return $prefix;
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
      . "adding \"$flag\" to ${executable}s parameters. To " . $prevent_or_allow
      . " this set " . "<TestFramework>->{add_pending_timeout} = "
      . ($self->{add_pending_timeout} ? "0" : "1") . "\n");
    $params .= $flag if $self->{add_pending_timeout};
  }
  my $proc = PerlDDS::create_process($executable, $params);
  $self->_track_log_files($params, $proc);
  return $proc;
}

sub _create_java_process {
  my $self = shift;
  my $main_class = shift;
  my $params = shift;
  my $jars = shift;
  my $vmargs = shift;

  $self->_info("TestFramework::_create_java_process creating executable="
    . "w/ params=$params jars=" . join(':', @{$jars}) . " vmargs=$vmargs\n");
  if ($params !~ /-DCPSPendingTimeout /) {
    my $flag = " -DCPSPendingTimeout 3 ";
    my $possible_would_be = ($self->{add_pending_timeout} ? "" : "would be ");
    my $prevent_or_allow = ($self->{add_pending_timeout} ? "prevent" : "allow");
    $self->_info("TestFramework::_create_process " . $possible_would_be
      . "adding \"$flag\" to parameters. To " . $prevent_or_allow
      . " this set " . "<TestFramework>->{add_pending_timeout} = "
      . ($self->{add_pending_timeout} ? "0" : "1") . "\n");
    $params .= $flag if $self->{add_pending_timeout};
  }
  my $proc = PerlDDS::create_java_process($main_class, $params, $jars, $vmargs);
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
  if ($self->{ini} ne "") {
      return $self->{ini};
  }

  if ($self->{transport} eq "") {
    trace("TestFramework::_ini_file should not be called if no transport has been identified");
  }
  my $transport = $self->{transport};
  unless (-e "$transport.ini") {
    my $alternate = _alternate_transport($transport);
    if ($alternate ne "" && -e "$alternate.ini") {
      $transport = $alternate;
    }
    elsif (-e "${name}_${transport}.ini") {
      $transport = $name . '_' . $transport;
    }
    else {
      my $transports = "$transport.ini";
      if ($alternate ne "") {
        $transports .= " or $alternate.ini";
      }
      trace("TestFramework::_init_file called but $transports "
        . "do not exist.  Either provide files, or set "
        . "<TestFramework>->{add_transport_config} = 0");
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

sub _info {
  my $self = shift;

  if ($self->{test_verbose}) {
    print STDERR $self->_log_prefix() . join(' ', @_);
  }
}

sub generate_governance {
    my $self = shift;
    my $key = shift;
    my @parts = split /_/, $key;
    my $output = shift;

    my $process = PerlACE::Process->new("$ENV{DDS_ROOT}/tests/security/attributes/gov_gen", "-aup $parts[0] -ejac $parts[1] -dpk $parts[2] -lpk $parts[3] -rpk $parts[4] -o $output -cert $ENV{DDS_ROOT}/tests/security/certs/permissions/permissions_ca_cert.pem -key $ENV{DDS_ROOT}/tests/security/certs/permissions/permissions_ca_private_key.pem");
    $process->Spawn();
    $process->WaitKill(5);
}

1;
