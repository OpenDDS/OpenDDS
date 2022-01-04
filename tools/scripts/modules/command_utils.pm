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
        @_,
    );
    my $name = $args{name} // $command_list[0];
    my $capture_stdout = $args{capture_stdout};
    my $verbose = $args{verbose} // $args{dry_run};
    my $dry_run = $args{dry_run};
    my $script_name = $args{script_name} ? "$args{script_name}: " : "";

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
        print STDERR "${script_name}ERROR: \"$name\" $error_message\n";
    }

    if (defined($capture_stdout) && $ran) {
        open($tmp_fd, $tmp_path);
        ${$capture_stdout} = do { local $/; <$tmp_fd> };
        close($tmp_fd);
    }

    return ($failed, $exit_status);
}

1;
