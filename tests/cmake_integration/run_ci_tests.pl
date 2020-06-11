#!/usr/bin/perl
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (travis-ci, azure-pipelines) but it can also
# be used to build and run the tests locally as long as DDS_ROOT and ACE_ROOT
# are set.
#

use Getopt::Long;
use Cwd;
use strict;

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
my $skip_cxx11;
my $no_shared;

exit 1 if !GetOptions(
    "build-config=s" => \$build_config,
    "generator=s" => \$generator,
    "arch=s" => \$arch,
    "skip-run-test" => \$skip_run_test,
    "skip-cxx11" => \$skip_cxx11,
    "no-shared" => \$no_shared,
    );

my @dirs = ('Messenger_1', 'Messenger_2');
push @dirs, 'C++11_Messenger' unless $skip_cxx11;

my %builds_lib = ('Messenger_2' => 1, 'C++11_Messenger' => 1);
my %runtest_in_config_dir = ('Messenger_1' => 1, 'Messenger_2' => 1);

for my $dir (@dirs) {
  my $build_dir="$ENV{'DDS_ROOT'}/tests/cmake_integration/Messenger/$dir/build";
  mkdir($build_dir) or die "ERROR '$!': failed to make directory $build_dir";
  chdir($build_dir) or die "ERROR: '$!': failed to switch to $build_dir";

  my @generate_cmd = ("cmake",
		      "-D", "CMAKE_PREFIX_PATH=$ENV{'DDS_ROOT'}",
		      "-D", "CMAKE_VERBOSE_MAKEFILE:BOOL=ON");
  my @build_cmd = ("cmake", "--build", ".");

  if ($generator ne "") {
    splice @generate_cmd, 1, 0, ("-G", qq("$generator"));
  }

  if ($arch ne "") {
    splice @generate_cmd, 1, 0, ("-A", "$arch");
  }

  if ($build_config ne "") {
    push @build_cmd, ("--config", "$build_config");
  }

  my @lib_options = ($builds_lib{$dir} && !$no_shared) ? ('OFF', 'ON') : ('');
  for my $lib_option (@lib_options) {

    my $lib_opt = $lib_option ? "-D BUILD_SHARED_LIBS=$lib_option" : '';
    run_command("@generate_cmd $lib_opt ..");
    run_command("@build_cmd");

    if (! $skip_run_test) {
      if ($build_config ne "" && $runtest_in_config_dir{$dir}) {
        my $run_dir = getcwd() . "/$build_config";
        print "Switching to '$run_dir' to run tests\n";
        chdir($run_dir) or die "ERROR: '$!'";
      }
      my $args = $build_config ? "-ExeSubDir $build_config -Config ARCH" : '';
      run_command("perl run_test.pl $args");
      if ($build_config ne "" && $runtest_in_config_dir{$dir}) {
        chdir($build_dir) or die "ERROR: '$!': failed to switch to $build_dir";
      }
    }
  }
}
