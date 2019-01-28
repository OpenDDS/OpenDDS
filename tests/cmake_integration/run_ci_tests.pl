#!/usr/bin/perl
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (travis-ci, azure-pipelines) but it can also
# be used to build and run the tests locally as long as DDS_ROOT and ACE_ROOT
# are set.
#

die "ERROR: DDS_ROOT must be set" if !$ENV{'DDS_ROOT'};
die "ERROR: ACE_ROOT must be set" if !$ENV{'ACE_ROOT'};

for my $x (qw(Messenger_1 Messenger_2)) {
  my $build_dir="$ENV{'DDS_ROOT'}/tests/cmake_integration/Messenger/$x/build";
  mkdir($build_dir) or die "ERROR '$!': failed to make directory $build_dir";
  chdir($build_dir) or die "ERROR: '$!': failed to switch to $build_dir";

  my @cmds = (["cmake",
               "-D", "CMAKE_PREFIX_PATH=$ENV{'DDS_ROOT'}",
               "-D", "CMAKE_VERBOSE_MAKEFILE:BOOL=ON", ".."],
              ["cmake", "--build", "."],
              ["./run_test.pl"]);

  for my $cmd (@cmds) {
    print "Running @{$cmd}\n";
    my $result = `@{$cmd} 2>&1`;

    if (defined $result) {
      print "$result\n";

    } else {
      die "ERROR: invoking @{$cmd} failed with error '$?'";
    }
  }
}
