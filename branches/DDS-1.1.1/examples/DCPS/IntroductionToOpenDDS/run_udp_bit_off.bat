start %DDS_ROOT%\bin\DCPSInfoRepo -NOBITS -ORBEndpoint iiop://localhost:12345 -d domain_ids
start subscriber -ORBSvcConf udp.conf -DCPSBit 0 -DCPSConfigFile sub_udp_conf.ini 
start subscriber -ORBSvcConf udp.conf -DCPSBit 0 -DCPSConfigFile sub2_udp_conf.ini 
start publisher -ORBSvcConf udp.conf -DCPSBit 0 -DCPSConfigFile pub_udp_conf.ini 