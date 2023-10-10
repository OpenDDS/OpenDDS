# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

cmake_minimum_required(VERSION 3.8.2)
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
project(opendds_generated_global_idl CXX)

find_package(OpenDDS REQUIRED)

if(NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "These tests require OpenDDS to be built with Ownership Profile enabled")
endif()

set(messenger "opendds_generated_global_messenger")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

configure_file("${src}/Messenger.idl" "${dst}/Messenger.idl" COPYONLY)

# Messenger library
add_library(${messenger} SHARED)
set_target_properties(${messenger}
  PROPERTIES OUTPUT_NAME messenger
)
opendds_target_sources(${messenger} PUBLIC "${dst}/Messenger.idl")
target_link_libraries(${messenger} OpenDDS::Dcps)
