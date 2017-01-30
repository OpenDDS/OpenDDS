#!/usr/bin/env bash

##############################################################################
# This script is used to create dds and tao IDL binaries which link only on
# libc.so, libm.so, linux-gate.so and ld-linux.so at runtime.
#
# Prerequesite: docker
##############################################################################

buildbits=32
docker_image=phusion/holy-build-box-${buildbits}
MOUNT_DIR=/OpenDDS

set -e

cd `dirname "${BASH_SOURCE[0]}"`/../..

## get the tao_version from the configure script
tao_version=`grep 'my $tao_version' ./configure | sed -r 's/my \\\$tao_version\s*=\s*//' | sed -r "s/'([^']+)'.+$/\1/"`

if [ ! -d ACE_wrappers ]; then
  curl http://download.ociweb.com/TAO-${tao_version}/ACE+TAO-${tao_version}_with_latest_patches_NO_makefiles.tar.gz | tar xz
fi


if [ ! -d ACE_wrappers/MPC ]; then
  if [ ! -f $MPC_ROOT/mwc.pl ]; then
    echo "Cannnot find MPC installation, please set the MPC_ROOT environment variable."
    exit 1
  else
    EXTRA_DOCKER_FLAGS="-v $MPC_ROOT:/MPC -e MPC_ROOT=/MPC"
  fi
else
  EXTRA_DOCKER_FLAGS=" -e MPC_ROOT=$MOUNT_DIR/ACE_wrappers/MPC"
fi


cat << 'EOF' > build_tao_dds_idl.sh
#!/bin/bash
set -e

# Activate Holy Build Box environment.
source /hbb_exe/activate

cd `dirname "${BASH_SOURCE[0]}"`

mkdir -p build

$MPC_ROOT/clone_build_tree.pl universal_linux_idls_build

export DDS_ROOT=$PWD/build/universal_linux_idls_build
export ACE_ROOT=$PWD/build/universal_linux_idls_build/ACE_wrappers
export TAO_ROOT=$PWD/build/universal_linux_idls_build/ACE_wrappers/TAO

cd $DDS_ROOT

files=( "ACE_wrappers/ace/config.h"
        "ACE_wrappers/include/makeinclude/platform_macros.GNU"
        "ACE_wrappers/apps/gperf/src/ace_gperf"
        "ACE_wrappers/TAO/TAO_IDL/tao_idl"
        "dds/idl/opendds_idl"
)

for i in "${files[@]}"
do
    if [ -h $i ]; then
      rm $i
    fi
done


if [ ! -f ACE_wrappers/ace/config.h ]; then
cat << 'EEOF' > ACE_wrappers/ace/config.h
#include "ace/config-linux.h"
#undef ACE_HAS_SVR4_DYNAMIC_LINKING
EEOF
fi

if [ ! -f ACE_wrappers/include/makeinclude/platform_macros.GNU ]; then
cat << EEOF > ACE_wrappers/include/makeinclude/platform_macros.GNU
static_libs_only = 1
buildbits = $buildbits
threads = 0
debug = 0
static_stdlibs = 1
dynamic_loader = 0
include \$(ACE_ROOT)/include/makeinclude/platform_linux.GNU
EEOF
fi

cat << 'EEOF' > tao_dds_idl.mwc
workspace {
  $(ACE_ROOT)/ace/ace.mpc
  $(ACE_ROOT)/apps/gperf/src
  $(TAO_ROOT)/TAO_IDL/tao_idl.mpc
  $(TAO_ROOT)/TAO_IDL/tao_idl_fe.mpc
  $(TAO_ROOT)/TAO_IDL/tao_idl_be.mpc
  $(DDS_ROOT)/dds/idl/opendds_idl.mpc
}
EEOF

ACE_wrappers/bin/mwc.pl -type gnuace tao_dds_idl.mwc
make

mkdir -p ../install/bin
cp ACE_wrappers/apps/gperf/src/ace_gperf ../install/bin
cp ACE_wrappers/TAO/TAO_IDL/tao_idl ../install/bin
cp dds/idl/opendds_idl ../install/bin

for f in ../install/bin/* ;
do
  strip $f
done

EOF

docker run -it --rm -u $UID -e "buildbits=$buildbits" -v $PWD:$MOUNT_DIR $EXTRA_DOCKER_FLAGS $docker_image bash $MOUNT_DIR/build_tao_dds_idl.sh

echo ""
echo "build is done, all binaries are in the $PWD/build/install/bin directory"

rm -rf build/universal_linux_idls_build build_tao_dds_idl.sh
