#!/bin/bash

set -e

if [ -e ${DDS_ROOT}/dds-devel.sh ]; then
  DDS_CP=${DDS_ROOT}/../../lib
else
  DDS_CP=${DDS_ROOT}/lib
fi

function on_exit {
  if [ ! -z "$(jobs -p)" ]; then
    kill $(jobs -p) >/dev/null 2>&1 || true
  fi
}
trap on_exit EXIT

#jobs &>/dev/null
#exec &>/dev/null

cd node_controller
./node_controller one-shot --name Leg_01 &
./node_controller one-shot --name Leg_02 &
./node_controller one-shot --name Leg_03 &
./node_controller one-shot --name Leg_04 &
./node_controller one-shot --name Leg_05 &
./node_controller one-shot --name Leg_06 &
./node_controller one-shot --name Leg_07 &
./node_controller one-shot --name Leg_08 &
cd ../test_controller
./test_controller ../example showtime_30 --wait-for-nodes 4
