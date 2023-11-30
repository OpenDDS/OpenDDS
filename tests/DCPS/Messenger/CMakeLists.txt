# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_tests_dcps_messenger CXX)
enable_testing()

find_package(OpenDDS REQUIRED ownership_profile)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")
set(publisher "${target_prefix}publisher")
set(subscriber "${target_prefix}subscriber")
set(stack_subscriber "${target_prefix}stack_subscriber")
set(messenger "${target_prefix}messenger")

# Messenger library
add_library(${messenger})
set_target_properties(${messenger}
  PROPERTIES OUTPUT_NAME messenger
)
opendds_target_sources(${messenger} PUBLIC Messenger.idl)
target_link_libraries(${messenger} PUBLIC OpenDDS::Dcps)

# Publisher
add_executable(${publisher} publisher.cpp)
set_target_properties(${publisher}
  PROPERTIES OUTPUT_NAME publisher
)
target_link_libraries(${publisher} ${messenger} OpenDDS::OpenDDS OpenDDS::TestUtils)

# Subscriber with heap-allocated entities
add_executable(${subscriber}
  subscriber.cpp
  DataReaderListener.h
  DataReaderListener.cpp
)
set_target_properties(${subscriber}
  PROPERTIES OUTPUT_NAME subscriber
)
target_link_libraries(${subscriber} ${messenger} OpenDDS::OpenDDS)

# Subscriber with stack-allocated entities
add_executable(${stack_subscriber}
  stack_subscriber.cpp
  DataReaderListener.h
  DataReaderListener.cpp
)
set_target_properties(${stack_subscriber}
  PROPERTIES OUTPUT_NAME stack_subscriber
)
target_link_libraries(${stack_subscriber} ${messenger} OpenDDS::OpenDDS)

# Testing
set(OPENDDS_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
opendds_add_test(NAME tcp)
opendds_add_test(NAME default_tcp ARGS default_tcp)
opendds_add_test(NAME thread_per ARGS thread_per)
if(OPENDDS_SUPPORTS_SHMEM)
  opendds_add_test(NAME shmem ARGS shmem)
endif()
opendds_add_test(NAME nobits ARGS nobits)
opendds_add_test(NAME stack ARGS stack)
if(OPENDDS_IPV6)
  opendds_add_test(NAME ipv6 ARGS ipv6)
endif()
opendds_add_test(NAME rtps ARGS rtps)
opendds_add_test(NAME rtps_unicast ARGS rtps_unicast)
opendds_add_test(NAME rtps_disc ARGS rtps_disc)
opendds_add_test(NAME rtps_disc_tcp ARGS rtps_disc_tcp)
opendds_add_test(NAME rtps_disc_tcp_thread_per ARGS rtps_disc_tcp thread_per)
opendds_add_test(NAME rtps_disc_tcp_udp ARGS rtps_disc_tcp_udp)
if(OPENDDS_SECURITY)
  opendds_add_test(NAME rtps_disc_half_sec_pub ARGS rtps_disc_half_sec_pub)
  opendds_add_test(NAME rtps_disc_half_sec_sub ARGS rtps_disc_half_sec_sub)
  opendds_add_test(NAME rtps_disc_sec ARGS rtps_disc_sec)
endif()
