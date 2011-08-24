// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

Instructions for Building the Example (assuming ACE, TAO, DDS, and MPC
are installed and configured):

1) Run Make Project Creator to generate build files:

Windows, VC 7.1:   perl %ACE_ROOT%\bin\mwc.pl -type vc71 StockQuoter.mwc
Unix, GNU Make:    $ACE_ROOT/bin/mwc.pl -type gnuace StockQuoter.mwc

2)  Build the application


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
