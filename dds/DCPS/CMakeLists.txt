# CMakeLists for OpenDDS_Util used by opendds_idl
cmake_minimum_required(VERSION 3.23...3.27)
project(opendds_util CXX)

set(deps ACE::ACE)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps})
include(opendds_build_helpers)

if(WIN32)
  set(opendds_library_args EXPORT_SYMBOLS_NAME OpenDDS_Dcps)
else()
  set(opendds_util_type STATIC)
  set(opendds_library_args NO_INSTALL)
endif()
add_library(OpenDDS_Util ${opendds_util_type}
  debug.cpp
  Hash.cpp
  SafetyProfileStreams.cpp
  Serializer.cpp
  XTypes/TypeObject.cpp
)
_opendds_library(OpenDDS_Util ${opendds_library_args})
target_compile_definitions(OpenDDS_Util PUBLIC OPENDDS_UTIL_BUILD)
target_include_directories(OpenDDS_Util
  PUBLIC
    "$<BUILD_INTERFACE:${OPENDDS_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
    .. # For pch
    ${TAO_INCLUDE_DIRS} # Only for orbconf.h and idl_features.h in Definitions.h
)
target_link_libraries(OpenDDS_Util PUBLIC ${deps})
