// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#############################
StockQuoter Optional tag Test
#############################

This test should create the StockQuoter example from OpenDDS-DDS-3.30/examples/DCPS/IntroductionToOpenDDS with a change to the .idl.
However, the build portion of this example will not work. 
The only difference is an "@optional" annotation inserted into the Quote Topic structure.

This is directly stemming from an already written example. 


Instructions for Building the Example (assuming ACE, TAO, DDS, and MPC
are installed and configured):

1) Run Make Project Creator to generate build files:

Windows, VC 7.1:   perl %ACE_ROOT%\bin\mwc.pl -type vc71 StockQuoter.mwc
Unix, GNU Make:    $ACE_ROOT/bin/mwc.pl -type gnuace StockQuoter.mwc

2)  Build the application


Instructions for running the example:

You may run "run_test.pl" which uses tcp or individually run each process everything below. 

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
