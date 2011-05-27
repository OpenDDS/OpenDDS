start %DDS_ROOT%\bin\DCPSInfoRepo -ORBSvcConf tcp.conf -ORBEndpoint iiop://localhost:12345 -d domain_ids
start subscriber -ORBSvcConf udp.conf -ORBSvcConf tcp.conf -DCPSConfigFile sub_udp_conf.ini 
start subscriber -ORBSvcConf udp.conf -ORBSvcConf tcp.conf -DCPSConfigFile sub2_udp_conf.ini 
start publisher -ORBSvcConf udp.conf -ORBSvcConf tcp.conf -DCPSConfigFile pub_udp_conf.ini 