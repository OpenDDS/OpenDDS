#!/bin/bash

if [ -e ${DDS_ROOT}/dds-devel.sh ]; then
  DDS_CP=${DDS_ROOT}/../../lib
else
  DDS_CP=${DDS_ROOT}/lib
fi

#jobs &>/dev/null
#exec &>/dev/null

cd node_controller
./node_controller --id 1 -DCPSConfigFile rtps_disc.ini &
./node_controller --id 2 -DCPSConfigFile rtps_disc.ini &
cd ../test_controller
sleep 5
./test_controller -DCPSConfigFile rtps_disc.ini
wait

