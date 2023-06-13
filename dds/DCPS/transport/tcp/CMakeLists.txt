project(opendds_tcp CXX)

set(deps OpenDDS::Dcps)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_Tcp
  TcpAcceptor.cpp
  TcpConnection.cpp
  Tcp.cpp
  TcpDataLink.cpp
  TcpInst.cpp
  TcpLoader.cpp
  TcpReceiveStrategy.cpp
  TcpSendStrategy.cpp
  TcpSynchResource.cpp
  TcpTransport.cpp

  TcpConnection.inl
  TcpDataLink.inl
  TcpInst.inl
  TcpReceiveStrategy.inl
)
_opendds_library(OpenDDS_Tcp)
target_link_libraries(OpenDDS_Tcp PUBLIC ${deps})
