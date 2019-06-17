sed -i "s/\"stop_time\": { \"sec\": \([-0-9]*\)/\"stop_time\": { \"sec\": -${1}/" configs/jfti_sim_daemon_config.json configs/jfti_sim_master_config.json configs/jfti_sim_worker_config.json
