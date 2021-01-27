#!/bin/bash

sed -i "s/\"stop_time\": { \"sec\": \([-0-9]*\)/\"stop_time\": { \"sec\": -${1}/" configs/showtime_daemon_config.json configs/showtime_master_config.json configs/showtime_worker_config.json
