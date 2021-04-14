# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

project(OpenDDS_car_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

if(NOT OpenDDS_FOUND)
  find_package(OpenDDS REQUIRED)
endif()

if(MSVC)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
endif()

set(OPENDDS_FILENAME_ONLY_INCLUDES ON CACHE BOOL "IDL files include other IDL files" FORCE)

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
)

# copy run_test.pl to build/
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/run_test.pl"
               "${CMAKE_CURRENT_BINARY_DIR}/run_test.pl"
               COPYONLY)

set(the_ecu "ecu")

add_executable(${the_ecu} "ecu.cpp")

add_library(ecu_lib)

OPENDDS_TARGET_SOURCES(ecu_lib ecu.idl
  ./engine/engine_specs.idl
  ./engine/engine_stats/fuel_stats.idl
  ./transmission/transmission_specs.idl
  ./transmission/transmission_states.idl)

target_link_libraries(ecu_lib ${opendds_libs})

target_link_libraries(${the_ecu} ${opendds_libs} ecu_lib)
