# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_tests_cmake_messenger_1 CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

if(NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "This test requires OpenDDS to be built with Ownership Profile enabled")
endif()

set(target_prefix "${PROJECT_NAME}_")
set(publisher "${target_prefix}publisher")
set(subscriber "${target_prefix}subscriber")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets ${publisher} ${subscriber})

foreach(file
  Messenger.idl subscriber.cpp publisher.cpp Args.h
  DataReaderListener.cpp DataReaderListener.h pub.ini sub.ini)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

# Publisher
add_executable(${publisher}
  "${dst}/publisher.cpp"
)
set_target_properties(${publisher}
  PROPERTIES OUTPUT_NAME publisher
)
target_include_directories(${publisher} PRIVATE "${src}/../../../")
opendds_target_sources(${publisher} "${dst}/Messenger.idl")

# Subscriber
add_executable(${subscriber}
  "${dst}/subscriber.cpp"
  "${dst}/DataReaderListener.h"
  "${dst}/DataReaderListener.cpp"
)
set_target_properties(${subscriber}
  PROPERTIES OUTPUT_NAME subscriber
)
opendds_target_sources(${subscriber} "${dst}/Messenger.idl")

foreach(t ${all_targets})
  # Using explicit scope here is a regression test for
  # https://github.com/OpenDDS/OpenDDS/issues/1336
  # See https://cmake.org/cmake/help/latest/policy/CMP0023.html
  target_link_libraries(${t} PUBLIC OpenDDS::OpenDDS)

  # Assert the mapping used was C++03
  get_property(mappings TARGET ${t} PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
  if(NOT "C++03" IN_LIST mappings)
    message(FATAL_ERROR "${t}: C++03 NOT in mapping list: ${mappings}")
  endif()
endforeach()

# Testing
configure_file("${src}/run_test.pl" . COPYONLY)
opendds_add_test(ASSUME_RUN_TEST)
