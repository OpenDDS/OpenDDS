#!/usr/bin/env bash

##############################################################################
# This script is used to create dds and tao IDL binaries which link only on
# libc.so , libm.so, linux-gate.so and ld-linux.so at runtime.
#
# Prerequesite: docker
##############################################################################

buildbits=32
docker_image=phusion/holy-build-box-${buildbits}

set -e

cd `dirname "${BASH_SOURCE[0]}"`/../..

## get the tao_version from the configure script
tao_version=`grep 'my $tao_version' ./configure | sed -r 's/my \\\$tao_version\s*=\s*//' | sed -r "s/'([^']+)'.+$/\1/"`

if [ ! -d ACE_wrappers ]; then
  curl http://download.ociweb.com/TAO-${tao_version}/ACE+TAO-${tao_version}_with_latest_patches_NO_makefiles.tar.gz | tar xz
fi

cat << 'EOF' > ACE_wrappers/ace/config.h
#include "ace/config-linux.h"
#undef ACE_HAS_SVR4_DYNAMIC_LINKING
EOF

cat << EOF > ACE_wrappers/include/makeinclude/platform_macros.GNU
static_libs_only = 1
buildbits = $buildbits
threads = 0
debug = 0
static_stdlibs = 1
dynamic_loader = 0
include \$(ACE_ROOT)/include/makeinclude/platform_linux.GNU
EOF

cat << 'EOF' > tao_dds_idl.mwc
workspace {
  $(ACE_ROOT)/ace/ace.mpc
  $(ACE_ROOT)/apps/gperf/src
  $(TAO_ROOT)/TAO_IDL/tao_idl.mpc
  $(TAO_ROOT)/TAO_IDL/tao_idl_fe.mpc
  $(TAO_ROOT)/TAO_IDL/tao_idl_be.mpc
  $(DDS_ROOT)/dds/idl/opendds_idl.mpc
}
EOF

MOUNT_DIR=/OpenDDS

cat << EOF > build_tao_dds_idl.sh
#!/bin/bash
set -e

# Activate Holy Build Box environment.
source /hbb_exe/activate

export DDS_ROOT=$MOUNT_DIR
export ACE_ROOT=$MOUNT_DIR/ACE_wrappers
export TAO_ROOT=$MOUNT_DIR/ACE_wrappers/TAO
export MPC_ROOT=$MOUNT_DIR/ACE_wrappers/MPC

# make sure no existing shared library exists;
# otherwise, the build would fail to link
rm -f ACE_wrappers/lib/*.so

cd $MOUNT_DIR
ACE_wrappers/bin/mwc.pl -type gnuace tao_dds_idl.mwc
make

EOF

docker run -it --rm -u $UID -v $PWD:$MOUNT_DIR $docker_image bash $MOUNT_DIR/build_tao_dds_idl.sh

