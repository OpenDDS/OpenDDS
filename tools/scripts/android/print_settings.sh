#!/usr/bin/env bash
# For Quickly Reproducing Failing CI Builds
source setenv.sh
cat << EOF
export arch=$arch
export ndk=$ndk
export api=$api
export use_toolchain=$use_toolchain
export ace_tao=$ace_tao
export use_security=$use_security
export use_java=$use_java
export target_api=$target_api
EOF
