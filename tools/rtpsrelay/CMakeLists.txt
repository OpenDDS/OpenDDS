cmake_minimum_required(VERSION 3.23...4.0)
project(opendds_rtps_relay CXX)

set(dep_libs
  OpenDDS::Rtps_Udp
  OpenDDS::Security
  OpenDDS::RtpsRelayLib
)
find_package(OpenDDS REQUIRED ${dep_libs} built_in_topics safety_profile=FALSE)
include(opendds_build_helpers)

add_executable(RtpsRelay
  Config.cpp
  GuidAddrSet.cpp
  GuidPartitionTable.cpp
  ParticipantListener.cpp
  ParticipantStatisticsReporter.cpp
  PublicationListener.cpp
  RelayAddressListener.cpp
  RelayConfigControlListener.cpp
  RelayHandler.cpp
  RelayHttpMetaDiscovery.cpp
  RelayParticipantStatusReporter.cpp
  RelayPartitionsListener.cpp
  RelayStatisticsReporter.cpp
  RelayStatusReporter.cpp
  RelayThreadMonitor.cpp
  RtpsRelay.cpp
  SubscriptionListener.cpp
)
_opendds_executable(RtpsRelay)
target_link_libraries(RtpsRelay PRIVATE ${dep_libs})

add_subdirectory(control)
