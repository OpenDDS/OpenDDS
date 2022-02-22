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

use lib "$DDS_ROOT/tools/scripts/modules/";
use command_utils qw//;

use Getopt::Long;

use constant windows => $^O eq "MSWin32";

my $gh_actions = ($ENV{GITHUB_ACTIONS} // "") eq "true";

my %os_configs = (
    MSWin32 => 'Win32',
    darwin => 'macOS',
    linux => 'Linux',
);

sub cd {
    my $dir = shift;
    chdir($dir) or die "auto_run_tests.pl: Error: Cannot chdir to $dir: $!";
}

my $dry_run = 0;

sub run_command {
    my $test_name = shift;
    return command_utils::run_command(@_,
        name => $test_name,
        script_name => 'auto_run_tests.pl',
        dry_run => $dry_run,
    );
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
    my $failed = run_command($name, $command, @_);
    mark_test_finish($name, time() - $start_time, $failed);
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
        "    --help | -h              Display this help\n" .
        "    --no-auto-config         Don't set common options by default. This includes\n" .
        "                             the RTPS and per-OS configs and the default test\n" .
        "                             lists below.\n";

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
        "    --cmake-cmd <cmd>        CMake command that is used internally\n" .
        "                             Tests still have to be built before hand\n" .
        "                             Default is `cmake`\n" .
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
        "    --show-all-configs       Same as --show-configs, but for all list files\n" .
        "    --list-configs           Print combined set of the configs from the list\n" .
        "                             files\n" .
        "    --list-all-configs       Same as --list-configs, but for all list files\n" .
        "    --list-tests             List all the tests that would run\n" .
        "    --list-all-tests         Same as --list-tests, but for all list files\n" .
        "    --stop-on-fail | -x      Stop on any failure\n";

    exit(0);
}

my $cmake_tests = "$DDS_ROOT/tests/cmake";

# Parse Options
my $help = 0;
my $auto_config = 1;
my $sandbox = '';
my $show_configs = 0;
my $show_all_configs = 0;
my $list_configs = 0;
my $list_all_configs = 0;
my $list_tests = 0;
my $list_all_tests = 0;
my $cmake = 0;
my $cmake_cmd = 'cmake';
my $cmake_build_dir = "$cmake_tests/build";
my $cmake_build_cfg = windows ? 'Debug' : undef;
my $ctest = 'ctest';
my $ctest_args = '';
# Python on Windows is called python from what I can see. On other platforms
# that are legacy-mindful python is Python 2, but we can rely on python3.
my $python = windows ? 'python' : 'python3';
my %opts = (
    'help|h' => \$help,
    'auto-config!' => \$auto_config,
    'sandbox|s=s' => \$sandbox,
    'dry-run|z' => \$dry_run,
    'show-configs' => \$show_configs,
    'show-all-configs' => \$show_all_configs,
    'list-configs' => \$list_configs,
    'list-all-configs' => \$list_all_configs,
    'list-tests' => \$list_tests,
    'list-all-tests' => \$list_all_tests,
    'stop-on-fail|x' => \$stop_on_fail,
    'cmake' => \$cmake,
    'cmake-cmd=s' => \$cmake_cmd,
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
my $query_all = $show_all_configs || $list_all_configs || $list_all_tests;
$show_configs |= $show_all_configs;
$list_configs |= $list_all_configs;
$list_tests |= $list_all_tests;
my $query = $show_configs || $list_configs || $list_tests;

# Determine what test list files to use
my @file_list = ();
if ($auto_config) {
    foreach my $list (@builtin_test_lists) {
        push(@file_list, "$DDS_ROOT/$list->{file}") if ($query_all || $list->{enabled});
    }
}
push(@file_list, @ARGV);

if ($auto_config) {
    die("auto_run_tests.pl: Error: unknown perl OS: $^O") if (!exists($os_configs{$^O}));
    push(@PerlACE::ConfigList::Configs, $os_configs{$^O});

    push(@PerlACE::ConfigList::Configs, "RTPS");
    if ($gh_actions) {
        push(@PerlACE::ConfigList::Configs, 'GH_ACTIONS');
    }
}

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

        run_test($test, $cmd);
    }
}

if ($cmake) {
    cd($cmake_build_dir);

    my $fake_name;
    my $process_name;
    my $process_func;
    my @process_cmd = (
        $python,
        "$cmake_tests/ctest-to-auto-run-tests.py",
        '--cmake', $cmake_cmd,
        $cmake_tests, '.'
    );
    my $art_output = 'art-output';
    if ($list_tests) {
        $process_name = "List CMake Tests";
        $process_func = \&run_command;
        push(@process_cmd, '--list');
    }
    else {
        $fake_name = "Run CMake Tests";
        mark_test_start($fake_name);
        my @run_test_cmd = ($ctest, '--no-compress-output', '-T', 'Test');
        if ($ctest_args) {
            push(@run_test_cmd, $ctest_args);
        }
        if ($ctest_args !~ /--build-config/ && defined($cmake_build_cfg)) {
            push(@run_test_cmd, "--build-config", $cmake_build_cfg);
        }
        run_test($fake_name, \@run_test_cmd, verbose => 1);
        $process_name = "Process CMake Test Results";
        $process_func = \&run_test;
        push(@process_cmd, '--art-output', $art_output);
        mark_test_start($process_name);
    }
    $process_func->($process_name, \@process_cmd);

    if (!$list_tests) {
        open(my $fh, $art_output) or die("Couldn't open $art_output: $!");
        while(<$fh>){
            print($_);
        }
        close($fh);
    }
}

# vim: expandtab:ts=4:sw=4
