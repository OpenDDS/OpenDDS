#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo build.sh =================================================================

source setenv.sh
source make.sh

# OpenDDS
pushd $workspace/../../.. > /dev/null
if [ -z "$host_tools" ]
then
  host_targets="TAO_IDL_EXE opendds_idl"
  target_targets="DDS_Messenger_Idl DDS_Messenger_Publisher DDS_Messenger_Subscriber"
  if $use_java
  then
    host_targets="$host_targets idl2jni_codegen"
    target_targets="$target_targets messenger_idl_test"
  fi
  pushd build/host > /dev/null
  $make $host_targets
  popd > /dev/null
  cd build/target
  $make $target_targets
else
  $make
fi
popd > /dev/null

# ACE Tests
if $build_ace_tests
then
  pushd $ace_target/tests > /dev/null
  old_ace_root="$ACE_ROOT"
  export ACE_ROOT="$ace_target"
  $make
  export ACE_ROOT="$old_ace_root"
  unset old_ace_root
  popd > /dev/null
fi
