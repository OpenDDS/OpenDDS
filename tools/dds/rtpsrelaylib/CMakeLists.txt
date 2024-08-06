cmake_minimum_required(VERSION 3.23...3.27)
project(opendds_rtps_relay_lib CXX)

set(deps
  OpenDDS::Dcps
)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_RtpsRelayLib
  Name.cpp
)
set_target_properties(OpenDDS_RtpsRelayLib PROPERTIES OUTPUT_NAME OpenDDS_RtpsRelay)
target_sources(OpenDDS_RtpsRelayLib
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}/tools" FILES
    Name.h
    PartitionIndex.h
    Utility.h
    export.h
)
_opendds_library(OpenDDS_RtpsRelayLib MSVC_BIGOBJ)
target_link_libraries(OpenDDS_RtpsRelayLib PUBLIC ${deps})

opendds_target_sources(OpenDDS_RtpsRelayLib
  PUBLIC
    Relay.idl
  INCLUDE_BASE "${OPENDDS_SOURCE_DIR}/tools"
  USE_EXPORT "dds/rtpsrelaylib/export.h" OpenDDS_RtpsRelayLib_Export
  OPENDDS_IDL_OPTIONS -Lc++11
)
opendds_install_interface_files(OpenDDS_RtpsRelayLib INCLUDE_BASE "${OPENDDS_SOURCE_DIR}/tools")
