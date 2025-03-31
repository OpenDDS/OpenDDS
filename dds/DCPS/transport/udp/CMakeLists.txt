cmake_minimum_required(VERSION 3.23...4.0)
project(opendds_udp CXX)

set(deps OpenDDS::Dcps)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_Udp
  Udp.cpp
  UdpDataLink.cpp
  UdpInst.cpp
  UdpLoader.cpp
  UdpReceiveStrategy.cpp
  UdpSendStrategy.cpp
  UdpTransport.cpp
)
target_sources(OpenDDS_Udp
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    Udp.h
    UdpDataLink.h
    UdpDataLink.inl
    UdpDataLink_rch.h
    UdpInst.h
    UdpInst_rch.h
    UdpLoader.h
    UdpReceiveStrategy.h
    UdpReceiveStrategy_rch.h
    UdpSendStrategy.h
    UdpSendStrategy_rch.h
    UdpTransport.h
    UdpTransport_rch.h
    Udp_Export.h
)
_opendds_library(OpenDDS_Udp)
target_link_libraries(OpenDDS_Udp PUBLIC ${deps})
