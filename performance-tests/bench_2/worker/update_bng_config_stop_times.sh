sed -i "s/\"stop_time\": { \"sec\": \([-0-9]*\)/\"stop_time\": { \"sec\": -${1}/" configs/bng_ops_center_config.json configs/bng_site_config.json
