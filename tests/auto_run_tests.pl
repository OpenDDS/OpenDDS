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
if ($@) {
  # TODO: Handle this
  die("Could not load configured_tests.pm. OpenDDS might not be configured be with --tests")
}

print("Configured lists:" . join(',', @configured_tests::lists) . "\n");
print("Configured configs: " . join(',', @configured_tests::includes) . "\n");
print("Configured excludes:" . join(',', @configured_tests::excludes) . "\n");
push(@PerlACE::ConfigList::Configs, @configured_tests::includes);
push(@PerlACE::ConfigList::Excludes, @configured_tests::excludes);

use Getopt::Long;
use Cwd;

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
        name => 'java',
        file => "java/tests/dcps_java_tests.lst",
    },
    {
        name => 'security',
        file => "tests/security/security_tests.lst",
    },
    {
        name => 'modeling',
        file => "tools/modeling/tests/modeling_tests.lst",
    },
);
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
        "    -l <list_file>           Include the tests from <list_file>\n" .

        "    --sandbox | -s <sandbox> Runs each program using a sandbox program\n" .
        "    --dry-run | -z           Just print the commands that would be ran\n" .
        "    --show-configs           Just print possible values for -Config\n" .
        "    --stop-on-fail | -x      Stop on any failure\n" .

        # These two are processed by PerlACE/ConfigList.pm
        "    -Config <cfg>            Include tests with <cfg> configuration\n" .
        "    -Exclude <cfg>           Exclude tests with <cfg> configuration\n" .

        # This one is processed by PerlACE/Process.pm
        "    -ExeSubDir <dir>         Subdirectory for finding the executables\n";
}

################################################################################

# Parse Options
my $help = 0;
my @l_options = ();
my $sandbox = '';
my $dry_run = 0;
my $show_configs = 0;
my $stop_on_fail = 0;
Getopt::Long::Configure('bundling', 'no_auto_abbrev');
my %opts = (
    'help|h' => \$help,
    'l=s' => \@l_options,
    'sandbox|s=s' => \$sandbox,
    'dry-run|z' => \$dry_run,
    'show-configs' => \$show_configs,
    'stop-on-fail|x' => \$stop_on_fail,
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

# Determine what test list files to use
my @file_list = ();
foreach my $list (@builtin_test_lists) {
    push(@file_list, "$DDS_ROOT/$list->{file}") if ($list->{enabled});
}
push(@file_list, @l_options);
push(@file_list, @ARGV);

if ($show_configs) {
    foreach my $test_list (@file_list) {
        my $config_list = new PerlACE::ConfigList;
        $config_list->load($test_list);
        print "$test_list: " . $config_list->list_configs() . "\n";
    }
    exit(0);
}

foreach my $test_lst (@file_list) {

    my $config_list = new PerlACE::ConfigList;
    $config_list->load($test_lst);

    # Ensures that we search for stuff in the current directory.
    $PATH .= $Config::Config{path_sep} . '.';

    foreach my $test ($config_list->valid_entries()) {
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
