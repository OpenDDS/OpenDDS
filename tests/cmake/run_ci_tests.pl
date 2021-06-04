#!/usr/bin/env perl
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (Github Actions) but it can also
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
my $compiler = "";
my $skip_run_test;
my $cxx_standard;
my $skip_typecode;
my $no_shared;

exit 1 if !GetOptions(
    "build-config=s" => \$build_config,
    "generator=s" => \$generator,
    "arch=s" => \$arch,
    "compiler=s" => \$compiler,
    "skip-run-test" => \$skip_run_test,
    "cxx-standard=s" => \$cxx_standard,
    "skip-typecode" => \$skip_typecode,
    "no-shared" => \$no_shared,
    );

my $skip_cxx11 = defined($cxx_standard) && $cxx_standard == 98;

my @dirs = ('../Nested_IDL', 'Messenger_1', 'Messenger_2');
push @dirs, '../generated_global' unless $no_shared;
push @dirs, 'C++11_Messenger' unless $skip_cxx11;
push @dirs, '../C++11_typecode' unless $skip_cxx11 || $skip_typecode;

my %builds_lib = ('Messenger_2' => 1, 'C++11_Messenger' => 1);

for my $dir (@dirs) {
  my $build_dir="$ENV{'DDS_ROOT'}/tests/cmake/Messenger/$dir/build";
  mkdir($build_dir) or die "ERROR '$!': failed to make directory $build_dir";
  chdir($build_dir) or die "ERROR: '$!': failed to switch to $build_dir";

  my @generate_cmd = ("cmake",
    "-D", "CMAKE_VERBOSE_MAKEFILE:BOOL=ON",
  );
  if ($compiler ne "") {
    push @generate_cmd, "-DCMAKE_CXX_COMPILER=$compiler";
  }
  if (defined($cxx_standard)) {
    push(@generate_cmd, "-DCMAKE_CXX_STANDARD=$cxx_standard");
  }

  my @base_build_cmd = ("cmake", "--build", ".");
  my @build_cmd = @base_build_cmd;

  if ($generator ne "") {
    splice @generate_cmd, 1, 0, ("-G", qq("$generator"));
  }

  if ($arch ne "") {
    splice @generate_cmd, 1, 0, ("-A", "$arch");
  }

  if ($build_config ne "") {
    push @build_cmd, ("--config", "$build_config");
  }
  if ($^O ne "MSWin32") {
    push(@build_cmd, "--", "-j2");
  }

  my @lib_options = ($builds_lib{$dir} && !$no_shared) ? ('OFF', 'ON') : ('');
  for my $lib_option (@lib_options) {

    my $lib_opt = $lib_option ? "-D BUILD_SHARED_LIBS=$lib_option" : '';
    run_command("@generate_cmd $lib_opt ..");
    run_command("@build_cmd");

    if (!$skip_run_test && $dir ne '../generated_global' && $dir ne '../C++11_typecode') {
      my $args = $build_config ? "-ExeSubDir $build_config -Config ARCH" : '';
      run_command("perl run_test.pl $args");
    }
    run_command("@base_build_cmd --target clean");
  }
}
