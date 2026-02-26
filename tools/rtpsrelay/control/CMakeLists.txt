cmake_minimum_required(VERSION 3.14...4.0)
project(opendds_rtps_relay_control CXX)

set(dep_libs
  OpenDDS::Rtps_Udp
  OpenDDS::Security
  OpenDDS::RtpsRelayLib
)
find_package(OpenDDS REQUIRED ${dep_libs} built_in_topics safety_profile=FALSE)
include(opendds_build_helpers)

add_executable(RtpsRelayControl
  RtpsRelayControl.cpp
)
_opendds_executable(RtpsRelayControl)
target_link_libraries(RtpsRelayControl PRIVATE ${dep_libs})
