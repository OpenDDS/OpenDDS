cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests_cxx11_messenger CXX)
enable_testing()

find_package(OpenDDS REQUIRED ownership_profile cxx11)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(opendds_libs
  OpenDDS::Dcps
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp
)

# IDL Library
set(idl "${target_prefix}idl")
add_library(${idl})
opendds_target_sources(${idl} PUBLIC Idl/Messenger.idl OPENDDS_IDL_OPTIONS -Lc++11)
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
set(idl_dir "${dst}/Idl")
set_target_properties(${idl} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${idl_dir}"
  LIBRARY_OUTPUT_DIRECTORY "${idl_dir}"
)

# Assert the mapping used was C++11
get_property(mappings TARGET ${idl} PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
if(NOT ("C++11" IN_LIST mappings))
  message(FATAL_ERROR "${idl}: C++11 NOT in mapping list: ${mappings}")
endif()

# Publisher
set(publisher "${target_prefix}publisher")
add_executable(${publisher}
  Publisher/publisher.cpp
)
set_target_properties(${publisher} PROPERTIES
  OUTPUT_NAME "publisher"
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Publisher"
)
target_link_libraries(${publisher} ${opendds_libs} ${idl})

# Subscriber
set(subscriber "${target_prefix}subscriber")
add_executable(${subscriber}
  Subscriber/subscriber.cpp
)
set_target_properties(${subscriber} PROPERTIES
  OUTPUT_NAME "subscriber"
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Subscriber"
)
target_link_libraries(${subscriber} ${opendds_libs} ${idl})

# Testing
opendds_add_test(EXTRA_LIB_DIRS "${idl_dir}")
