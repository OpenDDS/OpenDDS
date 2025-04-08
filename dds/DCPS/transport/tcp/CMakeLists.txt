cmake_minimum_required(VERSION 3.23...4.0)
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
)
target_sources(OpenDDS_Tcp
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    Tcp.h
    TcpAcceptor.h
    TcpConnection.h
    TcpConnection.inl
    TcpConnection_rch.h
    TcpDataLink.h
    TcpDataLink.inl
    TcpDataLink_rch.h
    TcpInst.h
    TcpInst.inl
    TcpInst_rch.h
    TcpLoader.h
    TcpReceiveStrategy.h
    TcpReceiveStrategy.inl
    TcpReceiveStrategy_rch.h
    TcpSendStrategy.h
    TcpSendStrategy_rch.h
    TcpSynchResource.h
    TcpTransport.h
    TcpTransport_rch.h
    Tcp_export.h
)
_opendds_library(OpenDDS_Tcp)
target_link_libraries(OpenDDS_Tcp PUBLIC ${deps})
