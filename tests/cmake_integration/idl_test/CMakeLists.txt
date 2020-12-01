# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

project(OpenDDS_car_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
)

set(the_ecu "ecu")

set(src "${CMAKE_CURRENT_BINARY_DIR}/..")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

foreach(file
  ecu.cpp ecu.idl)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

file(MAKE_DIRECTORY ${dst}/engine)
configure_file(${src}/engine/engine_specs.idl ${dst}/engine COPYONLY)

file(MAKE_DIRECTORY ${dst}/engine/engine_stats)
configure_file(${src}/engine/engine_stats/fuel_stats.idl ${dst}/engine/engine_stats COPYONLY)

file(MAKE_DIRECTORY ${dst}/transmission)
configure_file(${src}/transmission/transmission_specs.idl ${dst}/transmission COPYONLY)
configure_file(${src}/transmission/transmission_states.idl ${dst}/transmission COPYONLY)

add_executable(${the_ecu} "ecu.cpp")

add_library(ecu_lib SHARED)

OPENDDS_TARGET_SOURCES(ecu_lib ecu.idl
  ./engine/engine_specs.idl
  ./engine/engine_stats/fuel_stats.idl
  ./transmission/transmission_specs.idl
  ./transmission/transmission_states.idl
  FILENAME_ONLY_INCLUDES)

target_link_libraries(ecu_lib ${opendds_libs})

target_link_libraries(${the_ecu} ${opendds_libs} ecu_lib)
