#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo get_toolchain.sh =========================================================

source setenv.sh

if $need_toolchain && [ ! -d $toolchain_name ]
then
  echo "Generating Standalone Toolchain"
  py=
  if [ -x python2_source/python ]
  then
    py=python2_source/python
  else
    if command -v python2
    then
      py=python2
    fi
  fi
  $py $OPENDDS_ANDROID_NDK/build/tools/make_standalone_toolchain.py \
    --arch $arch --api $api --install-dir $toolchain_name
  echo "Done"
fi
