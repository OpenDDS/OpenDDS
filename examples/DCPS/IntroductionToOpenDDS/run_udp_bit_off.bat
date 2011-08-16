start %DDS_ROOT%\bin\DCPSInfoRepo -NOBITS -ORBEndpoint iiop://localhost:12345
start subscriber -DCPSBit 0 -DCPSConfigFile sub_udp_conf.ini 
start subscriber -DCPSBit 0 -DCPSConfigFile sub2_udp_conf.ini 
start publisher -DCPSBit 0 -DCPSConfigFile pub_udp_conf.ini 
