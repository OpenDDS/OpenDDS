#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo get_ace_tao.sh ===========================================================

source setenv.sh

cd $DDS_ROOT

if [ ! -d ACE_TAO ]
then
  git clone --depth 1 \
    ${ACE_TAO_REPO:-https://github.com/DOCGroup/ACE_TAO} \
    --branch ${ACE_TAO_BRANCH:-$ace_tao_default_branch}
fi

cd ACE_TAO

if [ ! -d MPC ]
then
  git clone --depth 1 https://github.com/DOCGroup/MPC
fi
