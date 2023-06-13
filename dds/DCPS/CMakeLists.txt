cmake_minimum_required(VERSION 3.22)

project(opendds_util CXX)

set(deps ACE::ACE)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps})
include(opendds_build_helpers)

add_library(OpenDDS_Util STATIC
  debug.cpp
  Hash.cpp
  SafetyProfileStreams.cpp
  Serializer.cpp
  XTypes/TypeObject.cpp
)
_opendds_library(OpenDDS_Util)
target_compile_definitions(OpenDDS_Util PUBLIC OPENDDS_UTIL_BUILD)
target_include_directories(OpenDDS_Util
  PUBLIC ../..
  PRIVATE
    .. # For pch
    ${TAO_INCLUDE_DIRS} # Only for orbconf.h and idl_features.h in Definitions.h
)
target_link_libraries(OpenDDS_Util ${deps})
