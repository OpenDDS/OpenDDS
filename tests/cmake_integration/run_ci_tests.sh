#!/bin/bash
#
# Script to build and run the CMake integration tests. This is primarily for
# continuous integration services (travis-ci, azure-pipelines) but it can also
# be used to build and run the tests locally.
#

set -ev
_error() {
  echo "ERROR: $1"
  false
}

[[ -n "$DDS_ROOT" ]] || _error "DDS_ROOT must be set."
[[ -n "$ACE_ROOT" ]] || _error "ACE_ROOT must be set."

for x in Messenger_1 Messenger_2; do
  build_dir=$DDS_ROOT/tests/cmake_integration/Messenger/$x/build
  mkdir -p $build_dir
  pushd $build_dir
  cmake -DCMAKE_PREFIX_PATH=$DDS_ROOT -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ..
  cmake --build .
  ./run_test.pl
  popd
done
