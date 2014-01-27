start %DDS_ROOT%\bin\DCPSInfoRepo -ORBEndpoint iiop://localhost:12345
start subscriber -DCPSConfigFile dds_udp_conf.ini 
start subscriber -DCPSConfigFile dds_udp_conf.ini 
start publisher -DCPSConfigFile dds_udp_conf.ini 
