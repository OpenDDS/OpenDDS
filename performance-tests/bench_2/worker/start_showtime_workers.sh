#!/bin/bash

pid_file="/tmp/bench_worker_pids"
log_file="/tmp/bench_worker_logs"

function gather_pids()
{
  new_job_started="$(jobs -n)"
  if [ -n "$new_job_started" ]; then
      echo $! >> ${pid_file}
      echo $1 >> ${log_file}
  fi
}

if [ -e $pid_file ]; then
  echo File $pid_file already exists. Services should already be started. PID file is owned by $(ls -la $pid_file | sed 's/ [ ]*/ /g' | cut -d ' ' -f 3).
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

j=1
for (( i=1; i<=site_count; i++ ))
do
  if [ x$do_mutrace = x1 ] && [ x$i = x1 ]; then
    log_file_name="/tmp/bench_worker_log_$j.txt"
    $mutrace_args ./worker configs/showtime_daemon_config.json --log $log_file_name &> mutrace_daemon_output.txt &
    gather_pids "$log_file_name"
    j=$( echo "$j + 1" | bc )
    log_file_name="/tmp/bench_worker_log_$j.txt"
    $mutrace_args ./worker configs/showtime_worker_config.json --log $log_file_name &> mutrace_worker_output.txt &
    gather_pids "$log_file_name"
    j=$( echo "$j + 1" | bc )
  else
    log_file_name="/tmp/bench_worker_log_$j.txt"
    ./worker configs/showtime_daemon_config.json --log $log_file_name &
    gather_pids "$log_file_name"
    j=$( echo "$j + 1" | bc )
    log_file_name="/tmp/bench_worker_log_$j.txt"
    ./worker configs/showtime_worker_config.json --log $log_file_name &
    gather_pids "$log_file_name"
    j=$( echo "$j + 1" | bc )
  fi
done

if [ x$do_mutrace = x1 ]; then
  log_file_name="/tmp/bench_worker_log_$j.txt"
  $mutrace_args ./worker configs/showtime_master_config.json --log $log_file_name &> mutrace_master_output.txt &
  gather_pids "$log_file_name"
  j=$( echo "$j + 1" | bc )
else
  log_file_name="/tmp/bench_worker_log_$j.txt"
  ./worker configs/showtime_master_config.json --log $log_file_name &
  gather_pids "$log_file_name"
  j=$( echo "$j + 1" | bc )
fi
