// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

Instructions for running the example:

For TCP pub/sub:

1)  Run the DCPSInfo server

       $DDS_ROOT/dds/InfoRepo/DCPSInfoRepo -ORBEndpoint iiop://localhost:12345 -d domain_ids

2)  Run the Subscriber

       ./subscriber -DCPSConfigFile dds_tcp_conf.ini


3)  Run the Publisher

       ./publisher -DCPSConfigFile dds_tcp_conf.ini


For UDP pub/sub:

1)  Run the DCPSInfo server

       $DDS_ROOT/dds/InfoRepo/DCPSInfoRepo -ORBEndpoint iiop://localhost:12345 -d domain_ids

2)  Run the Subscriber

       ./subscriber -DCPSConfigFile dds_udp_conf.ini


3)  Run the Publisher

       ./publisher -DCPSConfigFile dds_udp_conf.ini



-Don Busch
 busch_d@ociweb.com
