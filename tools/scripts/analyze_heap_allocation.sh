#!/bin/bash

echo -n "Number of calls to global operator new: ";
objdump -C -l -d -S ${DDS_ROOT}/lib/libOpenDDS_Corba.so ${DDS_ROOT}/lib/libOpenDDS_Dcps.so ${DDS_ROOT}/lib/libOpenDDS_FACE.so ${DDS_ROOT}/lib/libOpenDDS_Rtps.so ${DDS_ROOT}/lib/libOpenDDS_Rtps_Udp.so | grep '<operator new' | grep 'call' | wc -l
