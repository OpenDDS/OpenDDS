cmake_minimum_required(VERSION 3.23...4.0)
project(opendds_rtps_udp CXX)

set(deps OpenDDS::Dcps OpenDDS::Rtps)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_Rtps_Udp
  MetaSubmessage.cpp
  RtpsCustomizedElement.cpp
  RtpsSampleHeader.cpp
  RtpsTransportHeader.cpp
  RtpsUdp.cpp
  RtpsUdpDataLink.cpp
  RtpsUdpInst.cpp
  RtpsUdpLoader.cpp
  RtpsUdpReceiveStrategy.cpp
  RtpsUdpSendStrategy.cpp
  RtpsUdpTransport.cpp
  TransactionalRtpsSendQueue.cpp
)
target_sources(OpenDDS_Rtps_Udp
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    BundlingCacheKey.h
    ConstSharedRepoIdSet.h
    LocatorCacheKey.h
    MetaSubmessage.h
    RtpsCustomizedElement.h
    RtpsCustomizedElement.inl
    RtpsSampleHeader.h
    RtpsSampleHeader.inl
    RtpsTransportHeader.h
    RtpsTransportHeader.inl
    RtpsUdp.h
    RtpsUdpDataLink.h
    RtpsUdpDataLink.inl
    RtpsUdpDataLink_rch.h
    RtpsUdpInst.h
    RtpsUdpInst_rch.h
    RtpsUdpLoader.h
    RtpsUdpReceiveStrategy.h
    RtpsUdpReceiveStrategy_rch.h
    RtpsUdpSendStrategy.h
    RtpsUdpSendStrategy_rch.h
    RtpsUdpTransport.h
    RtpsUdpTransport_rch.h
    Rtps_Udp_Export.h
    TransactionalRtpsSendQueue.h
)
_opendds_library(OpenDDS_Rtps_Udp BIGOBJ)
target_link_libraries(OpenDDS_Rtps_Udp PUBLIC ${deps})
