# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

project(opendds_messenger_2 CXX)
cmake_minimum_required(VERSION 3.8.2)
enable_testing()

set(OPENDDS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
find_package(OpenDDS REQUIRED)
include(opendds_testing)

if(NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "This test requires OpenDDS to be built with Ownership Profile enabled")
endif()

set(target_prefix "${PROJECT_NAME}_")
set(publisher "${target_prefix}publisher")
set(subscriber "${target_prefix}subscriber")
set(messenger "${target_prefix}messenger")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets ${publisher} ${subscriber} ${messenger})

foreach(file
    Messenger.idl subscriber.cpp publisher.cpp Args.h
    DataReaderListener.cpp DataReaderListener.h)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

# Messenger library
add_library(${messenger})
set_target_properties(${messenger}
  PROPERTIES OUTPUT_NAME messenger
)
opendds_target_sources(${messenger} PUBLIC "${dst}/Messenger.idl")
target_link_libraries(${messenger} PUBLIC OpenDDS::Dcps)

# Publisher
add_executable(${publisher}
  "${dst}/publisher.cpp"
)
set_target_properties(${publisher}
  PROPERTIES OUTPUT_NAME publisher
)
target_link_libraries(${publisher} ${messenger} OpenDDS::OpenDDS OpenDDS::TestUtils)

# Subscriber
add_executable(${subscriber}
  "${dst}/subscriber.cpp"
  "${dst}/DataReaderListener.h"
  "${dst}/DataReaderListener.cpp"
)
set_target_properties(${subscriber}
  PROPERTIES OUTPUT_NAME subscriber
)
target_link_libraries(${subscriber} ${messenger} OpenDDS::OpenDDS)

# Testing
configure_file("${src}/run_test.pl" . COPYONLY)
set(OPENDDS_TEST_CONFIG_DIR "${src}")
_opendds_add_test(NAME tcp)
_opendds_add_test(NAME default_tcp ARGS default_tcp)
_opendds_add_test(NAME thread_per ARGS thread_per)
_opendds_add_test(NAME shmem ARGS shmem)
_opendds_add_test(NAME nobits ARGS nobits)
# _opendds_add_test(NAME stack ARGS stack)
# _opendds_add_test(NAME ipv6 ARGS ipv6)
_opendds_add_test(NAME rtps ARGS rtps)
_opendds_add_test(NAME rtps_unicast ARGS rtps_unicast)
_opendds_add_test(NAME rtps_disc ARGS rtps_disc)
_opendds_add_test(NAME rtps_disc_tcp ARGS rtps_disc_tcp)
_opendds_add_test(NAME rtps_disc_tcp_thread_per ARGS rtps_disc_tcp thread_per)
_opendds_add_test(NAME rtps_disc_tcp_udp ARGS rtps_disc_tcp_udp)
_opendds_add_test(NAME rtps_disc_half_sec_pub ARGS rtps_disc_half_sec_pub)
_opendds_add_test(NAME rtps_disc_half_sec_sub ARGS rtps_disc_half_sec_sub)
_opendds_add_test(NAME rtps_disc_sec ARGS rtps_disc_sec)
