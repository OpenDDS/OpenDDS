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

  RtpsCustomizedElement.inl
  RtpsSampleHeader.inl
  RtpsTransportHeader.inl
  RtpsUdpDataLink.inl
)
_opendds_library(OpenDDS_Rtps_Udp)
target_link_libraries(OpenDDS_Rtps_Udp PUBLIC ${deps})
