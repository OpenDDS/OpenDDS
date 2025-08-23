#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo configure.sh =============================================================

source setenv.sh

extra_configure_flags=()

if $use_java
then
  if [ -z "${jdk+x}" ]
  then
    extra_configure_flags+=("--java")
  else
    extra_configure_flags+=("--java=${jdk}")
  fi

  android_jar="$android_sdk/platforms/android-$android_target_api/android.jar"
  if [ ! -f "$android_jar" ]
  then
    echo "Error: $android_jar doesn't exist, check that android_sdk and android_target_api are correct" 1>&2
    exit 1
  fi

  extra_configure_flags+=("--macros=android_sdk:=$android_sdk"
                          "--macros=android_target_api:=$android_target_api")
fi

if $use_security
then
  extra_configure_flags+=("--xerces3=${XERCESCROOT}" "--openssl=${SSL_ROOT}" --security)
fi

if [ -n "$host_tools" ]
then
  extra_configure_flags+=("--host-tools=$host_tools" "--no-tests")
fi

if [ $ndk_major_rev -lt 16 ]
then
  extra_configure_flags+=(
    "--macros=__NDK_MINOR__:=$ndk_minor_rev"
    "--macros=__NDK_MAJOR__:=$ndk_major_rev"
    # platform_android.GNU should be defining this automatically, but that
    # doesn't seem to be working...
    '--configh=#define ACE_ANDROID_NDK_MISSING_NDK_VERSION_H'
  )
fi
if [ $ndk_major_rev -lt 15 ]
then
  extra_configure_flags+=("--macros=android_force_clang:=0")
fi

if [ ! -z "${force_cpp_std+x}" ]
then
  extra_configure_flags+=("--std=$force_cpp_std")
fi

if ! $use_toolchain
then
  extra_configure_flags+=(
    "--macros=android_ndk:=$OPENDDS_ANDROID_NDK"
    "--macros=android_api:=$api"
  )
fi

if $mpc_concurrent
then
  mpc_concurrent_arg="--mpc:workers $logical_cores"
fi

pushd $DDS_ROOT > /dev/null
./configure --target=android \
  --verbose \
  --ace=$ACE_ROOT \
  --tao=$TAO_ROOT \
  --tests \
  --no-inline \
  ${mpc_concurrent_arg:-} \
  --macros=ANDROID_ABI:=$abi \
  "${extra_configure_flags[@]}"
popd > /dev/null

if $build_ace_tests
then
  pushd $ace_target/tests > /dev/null
  old_ace_root="$ACE_ROOT"
  export ACE_ROOT="$ace_target"
  mwc.pl -type gnuace tests.mwc
  export ACE_ROOT="$old_ace_root"
  unset old_ace_root
  popd > /dev/null
fi
