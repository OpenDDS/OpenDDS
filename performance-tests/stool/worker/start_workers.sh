#!/bin/bash

if [ -e ${DDS_ROOT}/dds-devel.sh ]; then
  DDS_CP=${DDS_ROOT}/../../lib
else
  DDS_CP=${DDS_ROOT}/lib
fi

pid_file="/tmp/stool_worker_pids"

function gather_pids()
{
  new_job_started="$(jobs -n)"
  if [ -n "$new_job_started" ];then
      echo $! >> ${pid_file}
  fi
}

if [ -e $pid_file ]; then
  echo File $pid_file already exists. Services should already be started. PID file is owned by `ls -la $pid_file | sed 's/ [ ]*/ /g' | cut -d ' ' -f 3`.
  exit
fi

site_count=10
if [ -z ${1+"asdf"} ]; then
  site_count=10
else
  site_count=$1
fi

#echo "site count = $site_count"

jobs &>/dev/null
exec &>/dev/null

#LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BNG_ROOT}/lib
#PATH=${PATH}:${BNG_ROOT}/bin

for (( i=1; i<=$site_count; i++ ))
do
  ./worker configs/bng_site_config.json true &
  gather_pids
done
./worker configs/bng_ops_center_config.json true &
gather_pids
