package command_utils;

use strict;
use warnings;

use Cwd;
use POSIX qw(SIGINT);
use File::Temp qw(tempfile);

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
  run_command
);

# `run_command` runs a command like the `system` function built into Perl, but
# it also automatically prints out the reason that a command failed to run or
# the exit status of a command that returned something other than zero. It also
# checks if the program died on an interrupt signal (like when Crtl-C is used
# on Linux) and kills the script if that's the case.
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
#     } else {
#       print("Command failed to run\n");
#     }
#
# capture_stdout
#   Reference to scalar variable that will be set to a string that contatins
#   the capture of the stdout output of the program. stdout is redirected and
#   does not also appear in the perl script's stdout. Example:
#
#     my $stdout;
#     run_command(['command', 'arg', 'another arg'], capture_stdout => \$stdout);
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
# print_error
#     Boolean that when true prints error messages. Defaults to true.
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

  my %args = (
    name => undef,
    capture_stdout => undef,
    verbose => undef,
    dry_run => undef,
    script_name => undef,
    print_error => 1,
    exit_status => undef,
    @_,
  );
  my $name = $args{name} // $command_list[0];
  my $capture_stdout = $args{capture_stdout};
  my $verbose = $args{verbose} // $args{dry_run};
  my $dry_run = $args{dry_run};
  my $script_name = $args{script_name} ? "$args{script_name}: " : "";
  my $print_error = $args{print_error};
  my $exit_status_ref = $args{exit_status};

  if ($verbose) {
    my $cwd = getcwd();
    print "In \"$cwd\" ", $dry_run ? "would run" : "running ";
    if ($use_list) {
      print("(list):\n");
      for my $i (@command_list) {
        print(" - \"$i\"\n");
      }
    }
    else {
      print("(string): \"$command_str\"\n");
    }
    return (0, 0) if ($dry_run);
  }

  my $saved_stdout;
  my $tmp_fd;
  my $tmp_path;

  if (defined($capture_stdout)) {
    ($tmp_fd, $tmp_path) = File::Temp::tempfile(UNLINK => 1);
    open($saved_stdout, '>&', STDOUT);
    open(STDOUT, '>&', $tmp_fd);
  }

  my $failed = ($use_list ? system(@command_list) : system($command)) ? 1 : 0;
  my $system_status = $?;
  my $system_error = $!;
  my $ran = $system_status != -1;

  if (defined($capture_stdout)) {
    open(STDOUT, '>&', $saved_stdout);
    close($tmp_fd);
  }

  my $exit_status = 0;
  if ($failed) {
    $exit_status = $system_status >> 8;
    my $signal = $system_status & 127;
    die("${script_name}\"$name\" was interrupted") if ($signal == SIGINT);
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
    if ($print_error) {
      print STDERR "${script_name}ERROR: \"$name\" $error_message\n";
    }
  }

  if (defined($capture_stdout) && $ran) {
    open($tmp_fd, $tmp_path);
    ${$capture_stdout} = do { local $/; <$tmp_fd> };
    close($tmp_fd);
  }

  if (defined($exit_status_ref)) {
    ${$exit_status_ref} = $ran ? $exit_status : undef;
  }

  return $failed;
}

1;
