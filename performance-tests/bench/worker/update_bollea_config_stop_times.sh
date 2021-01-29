#!/bin/bash

sed -i "s/\"stop_time\": { \"sec\": \([-0-9]*\)/\"stop_time\": { \"sec\": -${1}/" configs/bollea_ops_center_config.json configs/bolero_site_config.json
