#!/bin/bash

if [ -e ${DDS_ROOT}/dds-devel.sh ]; then
  DDS_CP=${DDS_ROOT}/../../lib
else
  DDS_CP=${DDS_ROOT}/lib
fi

#jobs &>/dev/null
#exec &>/dev/null

cd node_controller
./node_controller --name Jessie &
./node_controller --name James &
cd ../test_controller
./test_controller &
wait

