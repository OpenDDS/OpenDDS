eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-
# This file is for running the run_test.pl scripts listed in
# auto_run_tests.lst.

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
if (defined $ENV{srcdir}) {
  use lib "$ENV{srcdir}/bin";
}
use PerlACE::Run_Test;

use English;
use Getopt::Std;
use Cwd;

use Env qw(DDS_ROOT ACE_ROOT PATH);

################################################################################

if (!getopts ('ds:cl:') || $opt_h) {
    print "auto_run_tests.pl [-a] [-h] [-s sandbox] [-o] [-t] [-l listfile]\n";
    print "\n";
    print "Runs the tests listed in dcps_tests.lst\n";
    print "\n";
    print "Options:\n";
    print "    -c config   Run the tests for the <config> configuration\n";
    print "    -h          display this help\n";
    print "    -s sandbox  Runs each program using a sandbox program\n";
    print "    -c          dcps tests only\n";
    print "    -Config cfg Run the tests for the <cfg> configuration\n";
    print "    -l listfile Run the tests specified in listfile instead of ".
        "dcps_tests.lst\n";
    print "\n";
    $dcps_config_list = new PerlACE::ConfigList;
    $dcps_config_list->load ($DDS_ROOT."/bin/dcps_tests.lst");
    print "DCPS Test Configs: " . $dcps_config_list->list_configs () . "\n";
    exit (1);
}

my @file_list;

if ($opt_l) {
    push (@file_list, $opt_l);
} else {
    push (@file_list, "$DDS_ROOT/bin/dcps_tests.lst");
}

foreach my $test_lst (@file_list) {

    my $config_list = new PerlACE::ConfigList;
    $config_list->load ($test_lst);

    # Ensures that we search for stuff in the current directory.
    $PATH .= $Config::Config{path_sep} . '.';

    foreach $test ($config_list->valid_entries ()) {
        my $directory = ".";
        my $program = ".";

        if ($test =~ /(.*)\/([^\/]*)$/) {
            $directory = $1;
            $program = $2;
        }
        else {
            $program = $test;
        }

        print "auto_run_tests: $test\n";

        chdir ($DDS_ROOT."/$directory")
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

        foreach my $config ($config_list->my_config_list ()) {
            $inherited_options .= " -Config $config ";
        }

        $cmd = '';
        if ($opt_s) {
            $program = "perl $program" if ($progNoArgs =~ /\.pl$/);
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
            $start_time = time();
            $result = system ($cmd);
            $time = time() - $start_time;

            if ($result > 0) {
                print "Error: $test returned with status $result\n";
            }

            print "\nauto_run_tests_finished: $test Time:$time"."s Result:$result\n";
        }
    }
}
