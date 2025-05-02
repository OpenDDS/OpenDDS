cmake_minimum_required(VERSION 3.23...4.0)
project(opendds_multicast CXX)

set(deps OpenDDS::Dcps)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_Multicast
  BestEffortSession.cpp
  BestEffortSessionFactory.cpp
  Multicast.cpp
  MulticastDataLink.cpp
  MulticastInst.cpp
  MulticastLoader.cpp
  MulticastReceiveStrategy.cpp
  MulticastSendStrategy.cpp
  MulticastSession.cpp
  MulticastSessionFactory.cpp
  MulticastTransport.cpp
  ReliableSession.cpp
  ReliableSessionFactory.cpp

  MulticastDataLink.inl
  MulticastSession.inl
)
target_sources(OpenDDS_Multicast
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    BestEffortSession.h
    BestEffortSessionFactory.h
    Multicast.h
    MulticastDataLink.h
    MulticastDataLink.inl
    MulticastDataLink_rch.h
    MulticastInst.h
    MulticastInst_rch.h
    MulticastLoader.h
    MulticastReceiveStrategy.h
    MulticastReceiveStrategy_rch.h
    MulticastSendStrategy.h
    MulticastSendStrategy_rch.h
    MulticastSession.h
    MulticastSession.inl
    MulticastSessionFactory.h
    MulticastSessionFactory_rch.h
    MulticastSession_rch.h
    MulticastTransport.h
    MulticastTransport_rch.h
    MulticastTypes.h
    Multicast_Export.h
    ReliableSession.h
    ReliableSessionFactory.h
)
_opendds_library(OpenDDS_Multicast)
target_link_libraries(OpenDDS_Multicast PUBLIC ${deps})
