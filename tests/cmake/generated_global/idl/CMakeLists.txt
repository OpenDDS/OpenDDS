# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

cmake_minimum_required(VERSION 3.28...4.0)
project(opendds_tests_cmake_generated_global_idl CXX)

find_package(OpenDDS REQUIRED ownership_profile)

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

configure_file("${src}/Messenger.idl" "${dst}/Messenger.idl" COPYONLY)

# Messenger library
add_library(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}
  PROPERTIES OUTPUT_NAME messenger
)
opendds_target_sources(${PROJECT_NAME}
  PUBLIC "${dst}/Messenger.idl"
  ALWAYS_GENERATE_LIB_EXPORT_HEADER TRUE
)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Dcps)
