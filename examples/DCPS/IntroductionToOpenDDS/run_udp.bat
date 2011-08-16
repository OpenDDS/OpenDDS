start %DDS_ROOT%\bin\DCPSInfoRepo -ORBEndpoint iiop://localhost:12345
start subscriber -DCPSConfigFile sub_udp_conf.ini 
start subscriber -DCPSConfigFile sub2_udp_conf.ini 
start publisher -DCPSConfigFile pub_udp_conf.ini 
