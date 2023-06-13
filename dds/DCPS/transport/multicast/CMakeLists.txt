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
_opendds_library(OpenDDS_Multicast)
target_link_libraries(OpenDDS_Multicast PUBLIC ${deps})
