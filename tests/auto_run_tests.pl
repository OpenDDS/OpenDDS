eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-
# This file is for running the run_test.pl scripts listed in
# auto_run_tests.lst.

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

use Getopt::Long;
use Cwd;
use Env qw(DDS_ROOT PATH);
use strict;

push(@PerlACE::ConfigList::Configs, 'NO_INFO_REPO');

################################################################################

my ($opt_h, $opt_d, $opt_a, $opt_x, $opt_s, @opt_l);

if (!GetOptions('h' => \$opt_h,
                'd' => \$opt_d,
                'a' => \$opt_a,
                'x' => \$opt_x,
                's=s' => \$opt_s,
                'l=s' => \@opt_l)
    || $opt_h) {
    print "auto_run_tests.pl [-a] [-h] [-s sandbox] [-o] [-t] [-l listfile]\n";
    print "\n";
    print "Runs the tests listed in dcps_tests.lst\n";
    print "\n";
    print "Options:\n";
    print "    -h          display this help\n";
    print "    -c config   Run the tests for the <config> configuration\n";
    print "    -Config cfg Run the tests for the <cfg> configuration\n";
    print "    -s sandbox  Runs each program using a sandbox program\n";
    print "    -a          Run all DDS (DCPS) tests (default unless -l)\n";
    print "    -x          Stop on any failure\n";
    print "    -l listfile Run the tests specified in list file\n";
    print "\n";
    my $dcps_config_list = new PerlACE::ConfigList;
    $dcps_config_list->load($DDS_ROOT . "/tests/dcps_tests.lst");
    print "DCPS Test Configs: " . $dcps_config_list->list_configs() . "\n";
    exit 1;
}

my @file_list;

if ($opt_a || !scalar @opt_l) {
    push(@file_list, "$DDS_ROOT/tests/dcps_tests.lst");
}

if (scalar @opt_l) {
    push(@file_list, @opt_l);
}

my $cwd = getcwd();

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

        my $start_time_string = localtime;
        print "auto_run_tests: $test\n";
        print "start $test at $start_time_string\n";

        chdir($DDS_ROOT."/$directory")
            || die "Error: Cannot chdir to $DDS_ROOT/$directory";

        my $subdir = $PerlACE::Process::ExeSubDir;
        my $progNoArgs = $program;
        if ($program =~ /(.*?) (.*)/) {
            $progNoArgs = $1;
            if (! -e $progNoArgs) {
                print STDERR "Error: $directory.$1 does not exist\n";
                next;
              }
          }
        else {
            my $cmd = $program;
            $cmd = $subdir.$cmd if ($program !~ /\.pl$/);
            if ((! -e $cmd) && (! -e "$cmd.exe")) {
                print STDERR "Error: $directory/$cmd does not exist\n";
                next;
              }
          }

        ### Genrate the -ExeSubDir and -Config options
        my $inherited_options = " -ExeSubDir $subdir ";

        foreach my $config ($config_list->my_config_list()) {
            $inherited_options .= " -Config $config ";
        }

        my $cmd = '';
        $program = "perl $program" if ($progNoArgs =~ /\.pl$/);
        if ($opt_s) {
            $cmd = "$opt_s \"$program $inherited_options\"";
        }
        else {
            $cmd = $program.$inherited_options;
            $cmd = $subdir.$cmd if ($progNoArgs !~ /\.pl$/);
        }

        my $result = 0;

        if (defined $opt_d) {
            print "Running: $cmd\n";
        }
        else {
            my $start_time = time();
            $result = system($cmd);
            my $time = time() - $start_time;

            if ($result != 0) {
                print "Error: $test returned with status $result\n";
                if ($opt_x) {
                    exit($result >> 8);
                }
            }

            my $stop_time_string = localtime;
            print "stop $test at $stop_time_string\n";
            print "\nauto_run_tests_finished: $test Time:$time"
                . "s Result:$result\n";
        }
    } #foreach $test
    chdir $cwd; #back to base dir
} #foreach $test_lst
