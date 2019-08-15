#!/bin/bash

pid_file="/tmp/bench_worker_pids"
log_file="/tmp/bench_worker_logs"

if [ -e $pid_file ]; then
  total_latency_samples=0;
  total_latency_sample_prods=0;
  total_jitter_samples=0;
  total_jitter_sample_prods=0;
  max_max_discovery=0;
  total_workers=0;
  total_undermatched=0;
  pid_count=$(wc -l ${pid_file} | cut -d ' ' -f 1);
  for i in $(seq 1 1 ${pid_count}); do
    pid=$(head -n ${i} ${pid_file} | tail -n 1)
    log=$(head -n ${i} ${log_file} | tail -n 1)
    kill $pid 2>/dev/null
    total_workers=$(echo "$total_workers + 1" | bc )
    undermatched_readers=$(cat ${log} | grep "undermatched readers" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
    undermatched_writers=$(cat ${log} | grep "undermatched writers" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
    total_undermatched=$( echo "$total_undermatched + $undermatched_readers + $undermatched_writers" | bc )
    max_discovery=$(cat ${log} | grep "max discovery time delta" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
    #echo max_discovery = $max_discovery
    max_max_discovery=$( echo "if ( $max_max_discovery > $max_discovery ) { $max_max_discovery; } else { $max_discovery; }" | bc )
    latency_sample_count_line=$(cat ${log} | grep "total (latency) sample count")
    if ! [ -z "$latency_sample_count_line" ]; then
      latency_sample_count=$(cat ${log} | grep "total (latency) sample count" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
      #echo latency_sample_count = $latency_sample_count
      total_latency_samples=$( echo "$total_latency_samples + $latency_sample_count" | bc )
      mean_latency=$(cat ${log} | grep "mean latency" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
      #echo mean_latency = $mean_latency
      total_latency_sample_prods=$( echo "$total_latency_sample_prods + ( $latency_sample_count * $mean_latency )" | bc )
    fi
    jitter_sample_count_line=$(cat ${log} | grep "total (jitter) sample count")
    if ! [ -z "$jitter_sample_count_line" ]; then
      jitter_sample_count=$(cat ${log} | grep "total (jitter) sample count" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
      #echo jitter_sample_count = $jitter_sample_count
      total_jitter_samples=$( echo "$total_jitter_samples + $jitter_sample_count" | bc )
      mean_jitter=$(cat ${log} | grep "mean jitter" | cut -d ':' -f 2 | sed 's/^ //g' | cut -d ' ' -f 1)
      #echo mean_jitter = $mean_jitter
      total_jitter_sample_prods=$( echo "$total_jitter_sample_prods + ( $jitter_sample_count * $mean_jitter )" | bc )
    fi
    rm -f ${log}
  done
  rm -f ${pid_file}
  rm -f ${log_file}
  echo "total workers = $total_workers"
  echo "total undermatched readers & writers = $total_undermatched"
  echo "maximum discovery time = $max_max_discovery"
  echo "total latency samples = $total_latency_samples"
  if [ $total_latency_samples == 0 ]; then
    echo "total mean latency = $total_latency_sample_prods / $total_latency_samples" = NaN
  else
    echo "total mean latency = $total_latency_sample_prods / $total_latency_samples" = $( echo "scale=6; $total_latency_sample_prods / $total_latency_samples" | bc )
  fi
  echo "total jitter samples = $total_jitter_samples"
  if [ $total_jitter_samples == 0 ]; then
    echo "total mean jitter = $total_jitter_sample_prods / $total_jitter_samples" = NaN
  else
    echo "total mean jitter = $total_jitter_sample_prods / $total_jitter_samples" = $( echo "scale=6; $total_jitter_sample_prods / $total_jitter_samples" | bc )
  fi
  echo "- - - - -"
else
  echo File $pid_file does not exist. Services do not currently appear to be running.
  exit
fi

