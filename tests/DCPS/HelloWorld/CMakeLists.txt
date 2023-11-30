cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_hello_world CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")

# IDL TypeSupport Library
set(idl "${target_prefix}idl")
add_library(${idl})
opendds_target_sources(${idl} PUBLIC "HelloWorld.idl")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp # For run_test.pl
  OpenDDS::Rtps OpenDDS::Rtps_Udp # For run_test.pl --rtps
  ${idl}
)

# Publisher
set(publisher "${target_prefix}publisher")
add_executable(${publisher} publisher.cpp)
target_link_libraries(${publisher} ${opendds_libs})
set_target_properties(${publisher} PROPERTIES OUTPUT_NAME publisher)

# Subscriber
set(subscriber "${target_prefix}subscriber")
add_executable(${subscriber} subscriber.cpp)
target_link_libraries(${subscriber} ${opendds_libs})
set_target_properties(${subscriber} PROPERTIES
  OUTPUT_NAME subscriber
)

# Testing
configure_file(rtps.ini . COPYONLY)
opendds_add_test(NAME info_repo)
opendds_add_test(NAME rtps ARGS ini=rtps.ini)
