eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Executes test list files (*.lst), which contain commands with conditions
# called configurations under which the commands are run.

use strict;
use warnings;

use Env qw(ACE_ROOT DDS_ROOT PATH);

use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use PerlDDS::Run_Test;

use Getopt::Long;
use Cwd;
use POSIX qw(SIGINT);
use File::Temp qw(tempfile);

use constant windows => $^O eq "MSWin32";

sub cd {
    my $dir = shift;
    chdir($dir) or die "auto_run_tests.pl: Error: Cannot chdir to $dir: $!";
}

sub run_command {
    my $what = shift;
    my $command = shift;
    my %args = (
        capture_stdout => undef,
        verbose => undef,
        dry_run => undef,
        @_,
    );
    my $capture_stdout = $args{capture_stdout};
    my $verbose = $args{verbose} // $args{dry_run};
    my $dry_run = $args{dry_run};

    if ($verbose) {
        my $cwd = getcwd();
        print "In \"$cwd\" ", $dry_run ? "would run" : "running", ":\n    $command\n";
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

    my $failed = system($command) ? 1 : 0;
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
        die("auto_run_tests.pl: \"$what\" was interrupted") if ($signal == SIGINT);
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
        print STDERR "auto_run_tests.pl: Error: \"$what\" $error_message\n";
    }

    if (defined($capture_stdout) && $ran) {
        open($tmp_fd, $tmp_path);
        ${$capture_stdout} = do { local $/; <$tmp_fd> };
        close($tmp_fd);
    }

    return ($failed, $exit_status);
}

sub mark_test_start {
    my $name = shift;
    print
        "\n==============================================================================\n" .
        "auto_run_tests: $name\n\n";
}

sub mark_test_finish {
    my $name = shift;
    my $time = shift;
    my $result = shift;
    print "\nauto_run_tests_finished: $name Time:${time}s Result:$result\n";
}

my $stop_on_fail = 0;

sub run_test {
    my $name = shift;
    my $command = shift;

    my $start_time = time();
    my ($failed, $exit_status) = run_command($name, $command, @_);
    mark_test_finish($name, time() - $start_time, $exit_status);
    exit(1) if ($failed && $stop_on_fail);
}

my @builtin_test_lists = (
    {
        name => 'dcps',
        file => "tests/dcps_tests.lst",
        default => 1,
    },
    {
        name => 'security',
        file => "tests/security/security_tests.lst",
    },
    {
        name => 'java',
        file => "java/tests/dcps_java_tests.lst",
    },
    {
        name => 'modeling',
        file => "tools/modeling/tests/modeling_tests.lst",
    },
);

sub print_usage {
    my $error = shift // 1;

    my $fd = $error ? *STDERR : *STDOUT;
    print $fd
        "auto_run_tests.pl [<options> ...] [<list_file> ...]\n" .
        "auto_run_tests.pl -h | --help\n" .
        "\n";
    if ($error) {
        print STDERR "ERROR: Use auto_run_tests.pl --help to see all the options\n";
        exit(1);
    }
}

sub print_help {
    print_usage(0);

    print
        "Executes test list files (*.lst), which contain commands with conditions called\n" .
        "configurations under which the commands are run.\n" .
        "\n" .
        "<list_file> can be a path or - to use stdin.\n" .
        "\n" .
        "Options:\n" .
        "    --help | -h              Display this help\n";

    my $indent = 29;
    foreach my $list (@builtin_test_lists) {
        my $prefix = "    --" . ($list->{default} ? "no-" : "");
        print sprintf("%s%-" . ($indent - length($prefix) - 1) . "s Include %s\n",
            $prefix, $list->{name}, $list->{file});
        print sprintf(" " x 29 . "%sncluded by default\n",
            $list->{default} ? "I" : "Not i");
    }

    print
        "    --cmake                  Run CMake Tests\n" .
        "                             Not included by default\n" .
        "    --cmake-build-dir <path> Path to the CMake tests binary directory\n" .
        "                             Default is \$DDS_ROOT/tests/cmake/build\n" .
        "    --cmake-build-cfg <cfg>  CMake build configuration, like the one passed to\n" .
        "                             `cmake --config`. Mostly used by Visual Studio.\n" .
        "                             Default is \"Debug\" on Windows, no default\n" .
        "                             elsewhere.\n" .
        "    --ctest <cmd>            CTest to use to run CMake Tests\n" .
        "                             Default is `ctest`\n" .
        "    --ctest-args <args>      Additional arguments to pass to CTest\n" .
        "    --python <cmd>           Python command to use to run\n" .
        "                             ctest-to-auto-run-tests.py.\n" .
        "                             Default is `python` on Windows, `python3`\n" .
        "                             elsewhere.\n" .

        # These two are processed by PerlACE/ConfigList.pm
        "    -Config <cfg>            Include tests with <cfg> configuration\n" .
        "    -Exclude <cfg>           Exclude tests with <cfg> configuration\n" .
        "                             This is parsed as a Perl regex and will always\n" .
        "                             override -Config regardless of the order\n" .

        # This one is processed by PerlACE/Process.pm
        "    -ExeSubDir <dir>         Subdirectory for finding the executables\n" .

        "    --sandbox | -s <sandbox> Runs each program using a sandbox program\n" .
        "    --dry-run | -z           Do everything except run the tests\n" .
        "    --show-configs           Print possible values for -Config and -Excludes\n" .
        "                             broken down by list file\n" .
        "    --list-configs           Print combined set of the configs from the list\n" .
        "                             files\n" .
        "    --list-tests             List all the tests that would run\n" .
        "    --stop-on-fail | -x      Stop on any failure\n";

    exit(0);
}

my $cmake_tests = "$DDS_ROOT/tests/cmake";

# Parse Options
my $help = 0;
my $sandbox = '';
my $dry_run = 0;
my $show_configs = 0;
my $list_configs = 0;
my $list_tests = 0;
my $cmake = 0;
my $cmake_build_dir = "$cmake_tests/build";
my $cmake_build_cfg = windows ? 'Debug' : undef;
my $ctest = 'ctest';
my $ctest_args = '';
# Python on Windows is called python from what I can see. On other platforms
# that are legacy-mindful python is Python 2, but we can rely on python3.
my $python = windows ? 'python' : 'python3';
my %opts = (
    'help|h' => \$help,
    'sandbox|s=s' => \$sandbox,
    'dry-run|z' => \$dry_run,
    'show-configs' => \$show_configs,
    'list-configs' => \$list_configs,
    'list-tests' => \$list_tests,
    'stop-on-fail|x' => \$stop_on_fail,
    'cmake' => \$cmake,
    'cmake-build-dir=s' => \$cmake_build_dir,
    'cmake-build-cfg=s' => \$cmake_build_cfg,
    'ctest=s' => \$ctest,
    'ctest-args=s' => \$ctest_args,
    'python=s' => \$python,
);
foreach my $list (@builtin_test_lists) {
    if (!exists($list->{default})) {
        $list->{default} = 0;
    }
    $list->{option} = undef;
    my $opt = "$list->{name}!";
    $opts{$opt} = \$list->{option};
}
Getopt::Long::Configure('bundling', 'no_auto_abbrev');
if (!GetOptions(%opts)) {
    print_usage(1);
}
elsif ($help) {
    print_help();
}
my $files_passed = scalar(@ARGV);
for my $list (@builtin_test_lists) {
    if (!exists($list->{enabled})) {
        $list->{enabled} = defined($list->{option}) ? $list->{option} :
            $files_passed ? 0 : $list->{default};
    }
}
my $query = $show_configs || $list_configs || $list_tests;

# Determine what test list files to use
my @file_list = ();
foreach my $list (@builtin_test_lists) {
    push(@file_list, "$DDS_ROOT/$list->{file}") if ($query || $list->{enabled});
}
push(@file_list, @ARGV);

if ($show_configs) {
    foreach my $test_list (@file_list) {
        my $config_list = new PerlACE::ConfigList;
        $config_list->load($test_list);
        print "$test_list: " . $config_list->list_configs() . "\n";
    }
    exit(0);
}

if ($list_configs) {
    my %configs = ();
    foreach my $test_list (@file_list) {
        my $config_list = new PerlACE::ConfigList;
        $config_list->load($test_list);
        for my $config (split(/ /, $config_list->list_configs())) {
            $configs{$config} = 1;
        }
    }
    for my $config (sort(keys(%configs))) {
        print "$config\n";
    }
    exit(0);
}

if (!$list_tests) {
    print "Test Lists: ", join(', ', @file_list), "\n";
    print "Configs: ", join(', ', @PerlACE::ConfigList::Configs), "\n";
    print "Excludes: ", join(', ', @PerlACE::ConfigList::Excludes), "\n";
}

foreach my $test_lst (@file_list) {

    my $config_list = new PerlACE::ConfigList;
    $config_list->load($test_lst);

    # Ensures that we search for stuff in the current directory.
    $PATH .= $Config::Config{path_sep} . '.';

    foreach my $test ($config_list->valid_entries()) {
        if ($list_tests) {
            print "$test\n";
            next;
        }

        my $directory = ".";
        my $program = ".";

        ## Remove intermediate '.' directories to allow the
        ## scoreboard matrix to read things correctly
        $test =~ s!/./!/!g;

        if ($test =~ /(.*)\/([^\/]*)$/) {
            $directory = $1;
            $program = $2;
        }
        else {
            $program = $test;
        }

        mark_test_start($test);

        cd("$DDS_ROOT/$directory");

        my $subdir = $PerlACE::Process::ExeSubDir;
        my $progNoArgs = $program;
        if ($program =~ /(.*?) (.*)/) {
            $progNoArgs = $1;
            if (! -e $progNoArgs) {
                print STDERR "auto_run_tests.pl: Error: $directory/$1 does not exist\n";
                next;
            }
        }
        else {
            my $cmd = $program;
            $cmd = $subdir.$cmd if ($program !~ /\.pl$/);
            if ((! -e $cmd) && (! -e "$cmd.exe")) {
                print STDERR "auto_run_tests.pl: Error: $directory/$cmd does not exist\n";
                next;
            }
        }

        ### Generate the -ExeSubDir and -Config options
        my $inherited_options = " -ExeSubDir $subdir ";

        foreach my $config ($config_list->my_config_list()) {
            $inherited_options .= " -Config $config ";
        }

        my $cmd = '';
        $program = "perl $program" if ($progNoArgs =~ /\.pl$/);
        if ($sandbox) {
            $cmd = "$sandbox \"$program $inherited_options\"";
        }
        else {
            $cmd = $program.$inherited_options;
            $cmd = $subdir.$cmd if ($progNoArgs !~ /\.pl$/);
        }

        run_test($test, $cmd, dry_run => $dry_run);
    }
}

if ($cmake) {
    cd($cmake_build_dir);

    my $fake_name = "Run CMake Tests";
    mark_test_start($fake_name) unless ($list_tests);
    my @cmd = ("$ctest");
    if ($dry_run || $list_tests) {
        push(@cmd, "--show-only");
    } else {
        push(@cmd, "--no-compress-output -T Test");
    }
    if ($ctest_args) {
        push(@cmd, $ctest_args);
    }
    if ($ctest_args !~ /--build-config/ && defined($cmake_build_cfg)) {
        push(@cmd, "--build-config $cmake_build_cfg");
    }
    if ($list_tests) {
        run_command($fake_name, join(' ', @cmd));
    } else {
        run_test($fake_name,  join(' ', @cmd), verbose => 1);

        $fake_name = "Process CMake Test Results";
        mark_test_start($fake_name);
        my $tests = "$DDS_ROOT/tests/cmake";
        my $output = "";
        run_test($fake_name, "$python $tests/ctest-to-auto-run-tests.py $tests .",
            dry_run => $dry_run,
            verbose => 1,
            capture_stdout => \$output);
        print($output);
    }
}

# vim: expandtab:ts=4:sw=4
