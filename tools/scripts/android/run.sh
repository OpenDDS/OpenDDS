#!/usr/bin/env bash

set -x
set -o pipefail
set -o errexit
set -o nounset

source settings.sh
use_security=${use_security:-false}

# Get Everything
bash get_ndk.sh
bash get_sdk.sh
bash get_toolchain.sh
bash get_ace_tao.sh
if $use_security
then
  bash get_xerces.sh
  bash get_openssl.sh
fi

# Configure and Build Security Dependencies
if $use_security
then
  bash build_xerces.sh
  bash build_openssl.sh
fi

# Configure and Build OpenDDS
bash configure.sh
bash build.sh
