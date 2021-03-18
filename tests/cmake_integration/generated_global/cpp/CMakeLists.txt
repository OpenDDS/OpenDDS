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

set(target_prefix "opendds_cmake_generate_")
set(publisher "${target_prefix}publisher")
set(messenger "${target_prefix}messenger")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../idl)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../idl)

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets ${publisher})

foreach(file
  publisher.cpp Writer.cpp Writer.h Args.h)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

# Publisher
add_executable(${publisher}
  "${dst}/publisher.cpp"
  "${dst}/Writer.h"
  "${dst}/Writer.cpp"
)

set_target_properties(${publisher}
  PROPERTIES OUTPUT_NAME publisher
)

get_source_file_property(prop "${CMAKE_CURRENT_BINARY_DIR}/../idl/${messenger}_export.h" GENERATED)
message("${messenger}_export.h GENERATED = ${prop}")
target_link_libraries(${publisher} ${messenger} OpenDDS::OpenDDS)
