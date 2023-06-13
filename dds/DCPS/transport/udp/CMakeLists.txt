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

  UdpDataLink.inl
)
_opendds_library(OpenDDS_Udp)
target_link_libraries(OpenDDS_Udp PUBLIC ${deps})
