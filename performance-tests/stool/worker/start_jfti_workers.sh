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
  if [ -n "$new_job_started" ]; then
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

echo "site count = $site_count"

jobs &>/dev/null
exec &>/dev/null

do_mutrace=0
mutrace_args="mutrace --hash-size=2000000"

for (( i=1; i<=$site_count; i++ ))
do
  if [ x$do_mutrace = x1 ] && [ x$i = x1 ]; then
    $mutrace_args ./worker configs/jfti_sim_daemon_config.json true &> mutrace_daemon_output.txt &
    gather_pids
    $mutrace_args ./worker configs/jfti_sim_worker_config.json true &> mutrace_worker_output.txt &
    gather_pids
  else
    ./worker configs/jfti_sim_daemon_config.json true &
    gather_pids
    ./worker configs/jfti_sim_worker_config.json true &
    gather_pids
  fi
done
if [ x$do_mutrace = x1 ]; then
  $mutrace_args ./worker configs/jfti_sim_master_config.json true &> mutrace_master_output.txt &
  gather_pids
else
  ./worker configs/jfti_sim_master_config.json true &
  gather_pids
fi
