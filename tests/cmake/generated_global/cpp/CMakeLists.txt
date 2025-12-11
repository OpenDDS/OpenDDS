# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

cmake_minimum_required(VERSION 3.8...4.0)
project(opendds_tests_cmake_generated_global_cpp CXX)

find_package(OpenDDS REQUIRED ownership_profile)

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

foreach(file publisher.cpp Args.h)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

add_executable(${PROJECT_NAME}
  "${dst}/publisher.cpp"
)
set_target_properties(${PROJECT_NAME}
  PROPERTIES OUTPUT_NAME publisher
)
target_link_libraries(${PROJECT_NAME}
  opendds_tests_cmake_generated_global_idl
  OpenDDS::OpenDDS
  OpenDDS_TestUtils
)
