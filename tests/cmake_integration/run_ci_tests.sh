#!/bin/bash
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (travis-ci, azure-pipelines) but it can also
# be used to build and run the tests locally.
#

set -ev
if [ -n "$1" ]; then
  DDS_ROOT="$1"
else
  echo "run_ci_tests.sh requires DDS_ROOT as first argument"
  false # trip the -e and return err
fi

for x in Messenger_1 Messenger_2; do
  build_dir=$DDS_ROOT/tests/cmake_integration/Messenger/$x/build
  mkdir -p $build_dir
  pushd $build_dir
  cmake -DCMAKE_PREFIX_PATH=$DDS_ROOT ..
  cmake --build .
  ./run_test.pl
  popd
done
