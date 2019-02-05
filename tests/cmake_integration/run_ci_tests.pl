#!/usr/bin/perl
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (travis-ci, azure-pipelines) but it can also
# be used to build and run the tests locally as long as DDS_ROOT and ACE_ROOT
# are set.
#

use Getopt::Long;
use Cwd;

die "ERROR: DDS_ROOT must be set" if !$ENV{'DDS_ROOT'};
die "ERROR: ACE_ROOT must be set" if !$ENV{'ACE_ROOT'};

my $build_config = "";
my $generator = "";
my $skip_run_test;
GetOptions("build-config=s" => \$build_config,
           "generator=s" => \$generator,
           "skip-run-test" => \$skip_run_test);

for my $x (qw(Messenger_1 Messenger_2)) {
  my $build_dir="$ENV{'DDS_ROOT'}/tests/cmake_integration/Messenger/$x/build";
  mkdir($build_dir) or die "ERROR '$!': failed to make directory $build_dir";
  chdir($build_dir) or die "ERROR: '$!': failed to switch to $build_dir";

  my @cmds = (["cmake",
               "-D", "CMAKE_PREFIX_PATH=$ENV{'DDS_ROOT'}",
               "-D", "CMAKE_VERBOSE_MAKEFILE:BOOL=ON", ".."],
              ["cmake", "--build", "."]);

  if ($generator ne "") {
    push @{$cmds[0]}, ("-G", qq("$generator"));
  }

  sub run_cmd {
    my $cmd = shift;
    print "Running @{$cmd}\n";
    my $result = `@{$cmd} 2>&1`;

    if (defined $result) {
      print "$result\n";

    } else {
      die "ERROR: invoking @{$cmd} failed with error '$?'";
    }
  }

  for my $cmd (@cmds) {
    run_cmd $cmd;
  }

  if (! $skip_run_test) {
    if ($build_config ne "") {
      my $run_dir = getcwd() . "/$build_config";
      print "Switching to '$run_dir' to run tests\n";
      print "$run_dir"
      chdir($run_dir) or die "ERROR: '$!'";
    }
    run_cmd ["perl", "run_test.pl"]
  }
}
