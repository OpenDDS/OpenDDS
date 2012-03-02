start %DDS_ROOT%\bin\DCPSInfoRepo -ORBEndpoint iiop://localhost:12345
start subscriber -DCPSConfigFile dds_tcp_conf.ini 
start subscriber -DCPSConfigFile dds_tcp_conf.ini 
start publisher -DCPSConfigFile dds_tcp_conf.ini 
