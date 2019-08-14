#!/usr/bin/perl
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (travis-ci, azure-pipelines) but it can also
# be used to build and run the tests locally as long as DDS_ROOT and ACE_ROOT
# are set.
#

use Getopt::Long;
use Cwd;

sub run_command ($) {
  my $command = shift;
  print "Running $command\n";
  if (system($command)) {
    my $error_message;
    if ($? == -1) {
      $error_message = "Failed to Run: $!";
    }
    elsif ($? & 127) {
      $error_message = sprintf("Exited on Signal %d", ($? & 127));
      $error_message .= " and Created Coredump" if ($? & 128);
    }
    else {
      $error_message = sprintf ("Returned %d", $? >> 8);
    }
    die "Command \"$command\" $error_message\n";
  }
}

die "ERROR: DDS_ROOT must be set" if !$ENV{'DDS_ROOT'};
die "ERROR: ACE_ROOT must be set" if !$ENV{'ACE_ROOT'};

my $build_config = "";
my $generator = "";
my $arch = "";
my $skip_run_test;

exit 1 if !GetOptions(
    "build-config=s" => \$build_config,
    "generator=s" => \$generator,
    "arch=s" => \$arch,
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
    splice @{$cmds[0]}, 1, 0, ("-G", qq("$generator"));
  }

  if ($arch ne "") {
    splice @{$cmds[0]}, 1, 0, ("-A", "$arch");
  }

  if ($build_config ne "") {
    push @{$cmds[1]}, ("--config", "$build_config");
  }

  for my $cmd (@cmds) {
    run_command("@{$cmd}");
  }

  if (! $skip_run_test) {
    if ($build_config ne "") {
      my $run_dir = getcwd() . "/$build_config";
      print "Switching to '$run_dir' to run tests\n";
      print "$run_dir";
      chdir($run_dir) or die "ERROR: '$!'";
    }
    run_command("perl run_test.pl");
  }
}
