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
use FindBin;
use lib "$FindBin::Bin";
eval {require configured_tests;};
my $have_configured_tests = $@ ? 0 : 1;

use Getopt::Long;
use Cwd;
use POSIX qw(SIGINT);

sub run_command {
    my $test = shift;
    my $command = shift;
    my $print_error = shift;

    my $result = 0;
    if (system($command)) {
        $result = $? >> 8;
        if ($print_error) {
            my $signal = $? & 127;
            my $coredump = $? & 128;
            my $error_message;
            if ($? == -1) {
                $error_message = "failed to run: $!";
            }
            elsif ($signal) {
                die("auto_run_tests: test interrupted") if ($signal == SIGINT);
                $error_message = sprintf("exited on signal %d", ($signal));
                $error_message .= " and created coredump" if ($coredump);
            }
            else {
                $error_message = sprintf("returned with status %d", $result);
            }
            print "auto_run_tests: Error: $test $error_message\n";
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

sub print_help {
    my $fd = shift;
    print $fd
        "auto_run_tests.pl [<options> ...] [<list_file> ...]\n" .
        "auto_run_tests.pl -h | --help\n" .
        "\n" .
        "Executes test list files (*.lst), which contain commands with conditions called\n" .
        "configurations under which the commands are run.\n" .
        "\n" .
        "Test lists files can be specified using the -l option or as non-option\n" .
        "arguments.\n" .
        "\n" .
        "Options:\n" .
        "    --help | -h              Display this help\n";

    my $indent = 29;
    foreach my $list (@builtin_test_lists) {
        my $prefix = "    --" . ($list->{default} ? "no-" : "");
        print $fd sprintf("%s%-" . ($indent - length($prefix) - 1) . "s Include %s\n",
            $prefix, $list->{name}, $list->{file});
        print $fd sprintf(" " x 29 . "%sncluded by default\n",
            $list->{default} ? "I" : "Not i");
    }

    print $fd
        "    --sandbox | -s <sandbox> Runs each program using a sandbox program\n" .
        "    --dry-run | -z           Do everything except run the tests\n" .
        "    --show-configs           Print possible values for -Config and -Excludes\n" .
        "                             broken down by list file\n" .
        "    --list-configs           Print combined set of the configs from the list\n" .
        "                             files\n" .
        "    --list-tests             List all the tests that would run\n" .
        "    --stop-on-fail | -x      Stop on any failure\n" .
        "    --no-auto-cfg            Don't automatically decide on test configurations,\n" .
        "                             which is done by default. If this is passed then\n" .
        "                             configurations must be set mannually\n" .

        # These two are processed by PerlACE/ConfigList.pm
        "    -Config <cfg>            Include tests with <cfg> configuration\n" .
        "    -Exclude <cfg>           Exclude tests with <cfg> configuration\n" .

        # This one is processed by PerlACE/Process.pm
        "    -ExeSubDir <dir>         Subdirectory for finding the executables\n";
}

# Parse Options
my $help = 0;
my $sandbox = '';
my $dry_run = 0;
my $show_configs = 0;
my $list_configs = 0;
my $list_tests = 0;
my $stop_on_fail = 0;
my $auto_cfg = 1;
Getopt::Long::Configure('bundling', 'no_auto_abbrev');
my %opts = (
    'help|h' => \$help,
    'sandbox|s=s' => \$sandbox,
    'dry-run|z' => \$dry_run,
    'show-configs' => \$show_configs,
    'list-configs' => \$list_configs,
    'list-tests' => \$list_tests,
    'stop-on-fail|x' => \$stop_on_fail,
    'auto-cfg!' => \$auto_cfg,
);
foreach my $list (@builtin_test_lists) {
    if (!exists($list->{default})) {
        $list->{default} = 0;
    }
    if (!exists($list->{enabled})) {
      $list->{enabled} = $list->{default};
    }
    $opts{"$list->{name}!"} = \$list->{enabled};
}
my $invalid_arguments = !GetOptions(%opts);
if ($invalid_arguments || $help) {
    print_help($invalid_arguments ? *STDERR : *STDOUT);
    exit($invalid_arguments ? 1 : 0);
}
my $query = $show_configs || $list_configs || $list_tests;

if ($auto_cfg) {
    if (($ENV{GITHUB_ACTIONS} || "") eq "true") {
        push(@PerlACE::ConfigList::Configs, 'GH_ACTIONS');

        if ($builtin_test_lists_hash{java}->{enabled}) {
            $builtin_test_lists_hash{modeling}->{enabled} = 1;
        }
    }

    if ($have_configured_tests) {
        print("Configured lists: " . join(',', @configured_tests::lists) . "\n")
          if (!$query);
        foreach my $list_name (@configured_tests::lists) {
            my $done = 0;
            foreach my $list (@builtin_test_lists) {
                if ($list->{name} eq $list_name) {
                    $list->{enabled} = 1;
                    $done = 1;
                    last;
                }
            }
            die "Invalid list_name \"$list_name\" from configured_tests::lists"
                if (!$done);
        }
        print("Configured configs: " . join(',', @configured_tests::includes) . "\n")
          if (!$query);
        push(@PerlACE::ConfigList::Configs, @configured_tests::includes);
        print("Configured excludes: " . join(',', @configured_tests::excludes) . "\n")
          if (!$query);
        push(@PerlACE::ConfigList::Excludes, @configured_tests::excludes)
    }
}

# Determine what test list files to use
my @file_list = ();
foreach my $list (@builtin_test_lists) {
    push(@file_list, "$DDS_ROOT/$list->{file}")
      if ($query || $list->{enabled});
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
        print("$config\n");
    }
    exit(0);
}

foreach my $test_lst (@file_list) {

    my $config_list = new PerlACE::ConfigList;
    $config_list->load($test_lst);

    # Ensures that we search for stuff in the current directory.
    $PATH .= $Config::Config{path_sep} . '.';

    foreach my $test ($config_list->valid_entries()) {
        if ($list_tests) {
            print("$test\n");
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

        chdir($DDS_ROOT."/$directory")
            || die "auto_run_tests: Error: Cannot chdir to $DDS_ROOT/$directory";

        my $subdir = $PerlACE::Process::ExeSubDir;
        my $progNoArgs = $program;
        if ($program =~ /(.*?) (.*)/) {
            $progNoArgs = $1;
            if (! -e $progNoArgs) {
                print STDERR "auto_run_tests: Error: $directory.$1 does not exist\n";
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
        if ($dry_run) {
            my $cwd = getcwd();
            print "In \"$cwd\" would run:\n    $cmd\n";
        }
        else {
            $result = run_command($test, $cmd, 1);
        }
        my $time = time() - $start_time;
        print "\nauto_run_tests_finished: $test Time:$time"."s Result:$result\n";
        print "==============================================================================\n";

        if ($result && $stop_on_fail) {
            exit(1);
        }
    }
}
