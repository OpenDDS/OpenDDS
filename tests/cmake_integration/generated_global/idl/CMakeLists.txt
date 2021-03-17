# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

project(opendds_generated_property CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)


set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})

if (NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "These tests require OpenDDS to be built with Ownership Profile enabled")
endif()

set(target_prefix "idl_")
set(messenger "${target_prefix}messenger")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

foreach(file
  Messenger.idl)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

# Messenger library
add_library(${messenger} SHARED)
set_target_properties(${messenger}
  PROPERTIES OUTPUT_NAME messenger
)
OPENDDS_TARGET_SOURCES(${messenger} "${dst}/Messenger.idl")
target_link_libraries(${messenger} OpenDDS::Dcps)
