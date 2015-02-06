start %DDS_ROOT%\bin\DCPSInfoRepo -NOBITS -ORBEndpoint iiop://localhost:12345
start subscriber -DCPSBit 0 -DCPSConfigFile dds_tcp_conf.ini 
start subscriber -DCPSBit 0 -DCPSConfigFile dds_tcp_conf.ini 
start publisher -DCPSBit 0 -DCPSConfigFile dds_tcp_conf.ini 
