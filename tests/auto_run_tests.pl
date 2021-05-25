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

my $dry_run = 0;

sub cd {
    my $dir = shift;
    chdir($dir) || die "auto_run_tests: Error: Cannot chdir to $dir";
}

sub run_command {
    my $what = shift;
    my $command = shift;
    my $print_error = shift // 1;

    if ($dry_run) {
        my $cwd = getcwd();
        print "In \"$cwd\" would run:\n    $command\n";
        return 0;
    }

    my $result = 0;
    if (system($command)) {
        $result = $? >> 8;
        my $signal = $? & 127;
        die("auto_run_tests: \"$what\" was interrupted") if ($signal == SIGINT);
        if ($print_error) {
            my $coredump = $? & 128;
            my $error_message;
            if ($? == -1) {
                $error_message = "failed to run: $!";
            }
            elsif ($signal) {
                $error_message = sprintf("exited on signal %d", ($signal));
                $error_message .= " and created coredump" if ($coredump);
            }
            else {
                $error_message = sprintf("returned with status %d", $result);
            }
            print "auto_run_tests: Error: $what $error_message\n";
        }
    }
    return $result;
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
my %builtin_test_lists_hash = map { $_->{name} => $_ } @builtin_test_lists;

sub print_usage {
    my $error = shift // 1;

    my $fd = $error ? *STDERR : *STDOUT;
    print $fd
        "auto_run_tests.pl [<options> ...] [<list_file> ...]\n" .
        "auto_run_tests.pl -h | --help\n" .
        "\n";
    if ($error) {
        print STDERR "Use auto_run_tests.pl --help to see all the options\n";
        exit(1);
    }
}

sub print_help {
    print_usage(0);

    print
        "Executes test list files (*.lst), which contain commands with conditions called\n" .
        "configurations under which the commands are run.\n" .
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
        "    --ctest <cmd>            CTest to use to run CMake Tests\n" .
        "                             Default is `ctest`\n" .
        "    --ctest-args <args>      Additional arguments to pass to CTest\n" .
        "    --python <cmd>           Python command to use to run\n" .
        "                             ctest-to-auto-run-tests.py.\n" .
        "                             Default is `python3`\n" .
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

# Parse Options
my $help = 0;
my $sandbox = '';
my $show_configs = 0;
my $list_configs = 0;
my $list_tests = 0;
my $stop_on_fail = 0;
my $cmake = 0;
my $cmake_build_dir = "$DDS_ROOT/tests/cmake/build";
my $ctest = 'ctest';
my $ctest_args = '';
my $python = 'python3';
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
foreach my $list (@file_list) {
    die("$list is not a readable file!") if (!-r $list);
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

        print "auto_run_tests: $test\n";

        cd("$DDS_ROOT/$directory");

        my $subdir = $PerlACE::Process::ExeSubDir;
        my $progNoArgs = $program;
        if ($program =~ /(.*?) (.*)/) {
            $progNoArgs = $1;
            if (! -e $progNoArgs) {
                print STDERR "auto_run_tests: Error: $directory/$1 does not exist\n";
                next;
            }
        }
        else {
            my $cmd = $program;
            $cmd = $subdir.$cmd if ($program !~ /\.pl$/);
            if ((! -e $cmd) && (! -e "$cmd.exe")) {
                print STDERR "auto_run_tests: Error: $directory/$cmd does not exist\n";
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

        my $result = 0;
        my $start_time = time();
        $result = run_command($test, $cmd);
        my $time = time() - $start_time;
        print "\nauto_run_tests_finished: $test Time:$time"."s Result:$result\n";
        print "==============================================================================\n";

        if ($result && $stop_on_fail) {
            exit(1);
        }
    }
}

if ($cmake) {
    cd($cmake_build_dir);

    if (run_command("CMake Tests", "$ctest --no-compress-output -T Test $ctest_args")) {
        exit(1) if ($stop_on_fail);
    }

    my $tests = "$DDS_ROOT/tests/cmake";
    if (run_command("Process CMake Test Results",
            "$python $tests/ctest-to-auto-run-tests.py $tests $cmake_build_dir")) {
        exit(1);
    }
}

# vim: expandtab:ts=4:sw=4
