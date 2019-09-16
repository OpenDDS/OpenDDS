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
./node_controller --name Jessie &
./node_controller --name James &
./node_controller --name Meowth &
cd ../test_controller
./test_controller &
wait
