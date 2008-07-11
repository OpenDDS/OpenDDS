start %DDS_ROOT%\bin\DCPSInfoRepo -NOBITS -ORBEndpoint iiop://localhost:12345 -d domain_ids
start subscriber -ORBSvcConf tcp.conf -DCPSBit 0 -DCPSConfigFile dds_tcp_conf.ini 
start subscriber -ORBSvcConf tcp.conf -DCPSBit 0 -DCPSConfigFile dds_tcp_conf.ini 
start publisher -ORBSvcConf tcp.conf -DCPSBit 0 -DCPSConfigFile dds_tcp_conf.ini 