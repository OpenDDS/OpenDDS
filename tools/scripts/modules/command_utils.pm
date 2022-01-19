package command_utils;

use strict;
use warnings;

use Cwd qw(getcwd);
use POSIX qw(SIGINT);
use File::Temp qw();
use File::Spec qw();
use Scalar::Util qw();

use FindBin;
use lib "$FindBin::RealBin";
use ChangeDir;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
  run_command
  get_dump_output
);

sub process_capture_arguments {
  my $capture = shift;

  my @capture_args = ();
  my $ref_type = ref($capture);
  if ($ref_type eq "HASH") {
    for my $key (sort(keys(%{$capture}))) {
      push(@capture_args, [[$key], $capture->{$key}]);
    }
  }
  else {
    push(@capture_args, [['stdout', 'stderr'], $capture]);
  }

  my %translate = (stdout => *STDOUT, stderr => *STDERR);
  my @capture_directives = ();
  for my $capture_arg (@capture_args) {
    my $std_fh_names = $capture_arg->[0];
    my $arg = $capture_arg->[1];

    # The possible destinations
    my $dest_var = undef;
    my $dest_fh = undef;
    my $dest_path = undef;

    # Examine arg for dest and any optional arguments
    my $ambiguous_dest = undef;
    my $keyword_args_in = {} ;
    my $type = ref(\$arg);
    if ($type eq "REF") {
      my $ref_type = ref($arg);
      if (Scalar::Util::openhandle($arg)) {
        $dest_fh = $arg;
      }
      elsif ($ref_type eq "SCALAR") {
        $dest_var = $arg;
      }
      elsif ($ref_type eq "ARRAY") {
        $ambiguous_dest = shift(@{$arg});
        my %arg_as_hash = (@{$arg});
        $keyword_args_in = \%arg_as_hash;
      }
      else {
        die("unexpected type $ref_type $type in argument");
      }
    }
    elsif ($type eq "SCALAR") { # Write to file or file handle
      $ambiguous_dest = $arg;
    }
    else {
      die("arguments to capture must be scalars or refs, not $type ($arg)");
    }

    # Examine ambiguous_dest
    if (!defined($dest_var) || !defined($dest_fh)) {
      my $ambiguous_dest_type = ref(\$ambiguous_dest);
      if ($ambiguous_dest_type eq "REF") {
        $dest_var = $ambiguous_dest;
      } else {
        if (defined($ambiguous_dest)) {
          if (Scalar::Util::openhandle($ambiguous_dest)) {
            $dest_fh = $ambiguous_dest;
          }
          else {
            $dest_path = $ambiguous_dest;
          }
        }
        else {
          my %kw = %{$keyword_args_in};
          if (exists($kw{dump_on_failure}) && $kw{dump_on_failure}) {
            my $just_for_this_case;
            $dest_var = \$just_for_this_case;
          } else {
            $dest_path = File::Spec->devnull();
          }
        }
      }
    }

    my @std_fhs = map { $translate{$_} } @{$std_fh_names};
    push(@capture_directives, {
      dest_var => $dest_var,
      dest_path => $dest_path,
      dest_fh => $dest_fh,
      passed_dest_fh => defined($dest_fh),
      std_fhs => \@std_fhs,
      std_fh_names => $std_fh_names,
      saved_fhs => [],
      dump_on_failure => 0,
      %{$keyword_args_in},
    });
  }

  return @capture_directives;
}

sub get_dump_output {
  my $cmd_name = shift;
  my $std_fh_names = join(', ', @{shift()});
  my $capture_ref = shift;

  my $start = "Start of dump of $std_fh_names  for $cmd_name ";
  my $end = "End of dump of $std_fh_names for $cmd_name ";
  return $start . ("-" x (80 - length($start))) . "\n" .
    ${$capture_ref} . "\n" .
    $end . ("-" x (80 - length($end))) . "\n";
}

sub die_with_stack_trace {
  my $i = 1;
  print STDERR ("ERROR: ", @_, " STACK TRACE:\n");
  while (my @call_details = (caller($i++)) ){
      print STDERR "ERROR: STACK TRACE[", $i - 2, "] " .
        "$call_details[1]:$call_details[2] in function $call_details[3]\n";
  }
  die();
}

# `run_command` runs a command using the `system` function built into Perl, but
# with extra features. It automatically prints out the exit status of a command
# that returned something other than zero. It also checks if the program died
# on an interrupt signal (like when Crtl-C is used on Linux) and kills the
# script if that's the case.
#
# Note that because `run_command` is exported using `@EXPORT_OK`, it must be
# explicitly referenced using one of the following:
#
#   use command_utils qw/run_command/;
#   run_command('do-the-thing');
#
#   use command_utils;
#   command_utils::run_command('do-the-thing');
#
# The rational for this is that `run_command` can be wrapped in another
# `run_command` function to pass arguments that are specific to the local
# script.
#
# The arguments are the command, which can either be a string of the command or
# a reference to an array of the `argv` elements of the command, followed by
# optional arguments. Examples:
#
#   run_command('do-the-thing arg1 arg2');
#   run_command(['command', 'arg', 'another arg']);
#   my @cmd = ('command', 'arg', 'another arg');
#   run_command(\@cmd);
#
# 0 is returned if the command ran and returned a 0 exit status, else 1 is
# returned.
#
# The optional arguments are passed as a hash key value elements directly
# inside the arguments. Example:
#
#   run_command('do-the-thing', verbose => 1, script_name => 'doer');
#
# The optional arguments are:
#
# exit_status
#   The unsigned 8-bit exit status of the program or undef if the command
#   failed to run. Example:
#
#     my $exit_status
#     run_command('do-the-thing', exit_status => \$exit_status);
#     if (defined($exit_status)) {
#       print("Command ran\nexit status: $exit_status\n");
#     }
#     else {
#       print("Command failed to run\n");
#     }
#
# chdir
#   Change to the given directory before executing the command, returning
#   afterwards. Any other path to passed will be relative to this one. Default
#   is undef, which does not change directories.
#
# capture
#   Instead of outputting to the stdout and stderr of the script, capture or
#   redirect it to a variable ref, file path, file handle, or nothing (undef).
#   These are some simple examples of redirecting both stdout and stderr to a
#   variable or file path destination:
#
#     my $output;
#     run_command('do-the-thing', capture => \$output);
#     print("The output is $output\n");
#
#     run_command('do-the-thing', capture => "thing.log");
#
#   `run_command` can also automatically print the output if the command
#   failed. This cannot be done if a file handle destination is passed, but can
#   be done with any other kind of destination. For example:
#
#     run_command('do-the-thing', capture => [undef, dump_on_failure => 1]);
#
#     run_command('do-the-thing', capture => ["thing.log", dump_on_failure => 1]);
#
#   Finally stdout and stderr can be controlled individually by passing a hash
#   ref with the stream names containing same individual settings as above.
#   In this this example we are silencing stdout, while logging stderr if the
#   command fails:
#
#     run_command('do-the-thing', capture => {
#       stdout => undef,
#       stderr => [undef, dump_on_failure => 1],
#     });
#
# name
#     String of the name of the command to report in errors. Defaults to the
#     first element of the command array.
#
# verbose
#     Boolean that when true prints the command and current working directory
#     before running. Defaults to false.
#
# dry_run
#     The same as verbose, but doesn't run the command. Defaults to false.
#
# script_name
#     String of the name of the perl script running the command to report in
#     errors. Defaults to being blank.
#
# error_fh
#     File handle to print failure messages and capture dump_on_failure output
#     to. Setting to undef disables these.
#
sub run_command {
  my $command = shift;
  my @command_list;
  my $command_str;
  my $use_list = ref($command);
  if ($use_list) {
    @command_list = @{$command};
    $command_str = join(' ', @command_list);
  }
  else {
    @command_list = split(/ /, $command);
    $command_str = $command;
  }

  my %valid_args = (
    name => undef,
    capture => {},
    verbose => 0,
    verbose_fh => *STDERR,
    dry_run => 0,
    script_name => undef,
    error_fh => *STDERR,
    exit_status => undef,
    chdir => undef,
  );

  my %args = (%valid_args, @_);
  my @invalid_args = grep { !exists($valid_args{$_}) } keys(%args);
  die_with_stack_trace("invalid arguments: ", join(', ', @invalid_args)) if (scalar(@invalid_args));

  my $chdir = ChangeDir->new($args{chdir});

  my $name = $args{name} // $command_list[0];
  my @capture_directives = process_capture_arguments($args{capture});
  my $verbose = $args{verbose} // $args{dry_run};
  my $verbose_fh = $args{verbose_fh};
  my $dry_run = $args{dry_run};
  my $script_name = $args{script_name} ? "$args{script_name}: " : "";
  my $error_fh = $args{error_fh};
  my $exit_status_ref = $args{exit_status};

  if ($verbose && defined($verbose_fh)) {
    my $cwd = getcwd();
    print $verbose_fh ("In \"$cwd\" ", $dry_run ? "would run" : "running ");
    if ($use_list) {
      print $verbose_fh ("(list):\n");
      for my $i (@command_list) {
        print $verbose_fh (" - \"$i\"\n");
      }
    }
    else {
      print $verbose_fh ("(string): \"$command_str\"\n");
    }
    return 0 if ($dry_run);
  }

  # Start captures
  for my $capture_directive (@capture_directives) {
    if (defined($capture_directive->{dest_var})) {
      my ($tmp_fh, $tmp_path) = File::Temp::tempfile(UNLINK => 1);
      $capture_directive->{dest_fh} = $tmp_fh;
      $capture_directive->{dest_path} = $tmp_path;
    }
    elsif (defined($capture_directive->{dest_fh})) {
      if ($capture_directive->{dump_on_failure}) {
        die_with_stack_trace(
          "dump_on_failure requires a variable, path, or undef destination, not a file handle");
      }
    }
    elsif (defined($capture_directive->{dest_path})) {
      open($capture_directive->{dest_fh}, '>', $capture_directive->{dest_path})
          or die_with_stack_trace("failed to open $capture_directive->{dest_path}: $!");
    }
    else {
      die_with_stack_trace("capture_directive is invalid");
    }
    for my $std_fh (@{$capture_directive->{std_fhs}}) {
      open(my $saved_fh, '>&', $std_fh);
      open($std_fh, '>&', $capture_directive->{dest_fh});
      push(@{$capture_directive->{saved_fhs}}, $saved_fh);
    }
  }

  # Run command
  my $failed;
  {
    no warnings 'exec';
    $failed = ($use_list ? system(@command_list) : system($command)) ? 1 : 0;
  }
  my $system_status = $?;
  my $system_error = $!;
  my $ran = $system_status != -1;

  # Reverse redirect
  for my $capture_directive (@capture_directives) {
    for (my $i = 0; $i < scalar(@{$capture_directive->{std_fhs}}); $i++) {
      open($capture_directive->{std_fhs}->[$i], '>&', $capture_directive->{saved_fhs}->[$i]);
    }
    if (!$capture_directive->{passed_dest_fh}) {
      close($capture_directive->{dest_fh});
    }
    if ($ran && defined($capture_directive->{dest_var})) {
      open(my $fh, $capture_directive->{dest_path});
      ${$capture_directive->{dest_var}} = do { local $/; <$fh> };
      close($fh);
    }
  }

  # Process results
  my $exit_status = 0;
  if ($failed) {
    # Dump output if ran and directed to do so
    if ($ran) {
      for my $capture_directive (@capture_directives) {
        if ($capture_directive->{dump_on_failure} && defined($error_fh)) {
          print $error_fh get_dump_output(
            $name, $capture_directive->{std_fh_names}, $capture_directive->{dest_var});
        }
      }
    }

    $exit_status = $system_status >> 8;
    my $signal = $system_status & 127;
    die_with_stack_trace("${script_name}\"$name\" was interrupted") if ($signal == SIGINT);
    my $coredump = $system_status & 128;
    my $error_message;
    if (!$ran) {
      $error_message = "failed to run: $system_error";
    }
    elsif ($signal) {
      $error_message = sprintf("exited on signal %d", ($signal));
      $error_message .= " and created coredump" if ($coredump);
    }
    else {
      $error_message = sprintf("returned with status %d", $exit_status);
    }
    if (defined($error_fh)) {
      print $error_fh "${script_name}ERROR: \"$name\" $error_message\n";
    }
  }

  if (defined($exit_status_ref)) {
    ${$exit_status_ref} = $ran ? $exit_status : undef;
  }

  return $failed;
}

1;
