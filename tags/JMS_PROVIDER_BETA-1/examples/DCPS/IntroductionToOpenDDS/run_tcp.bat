start %DDS_ROOT%\bin\DCPSInfoRepo -ORBSvcConf tcp.conf -ORBEndpoint iiop://localhost:12345
start subscriber -ORBSvcConf tcp.conf -DCPSConfigFile dds_tcp_conf.ini 
start subscriber -ORBSvcConf tcp.conf -DCPSConfigFile dds_tcp_conf.ini 
start publisher -ORBSvcConf tcp.conf -DCPSConfigFile dds_tcp_conf.ini 
