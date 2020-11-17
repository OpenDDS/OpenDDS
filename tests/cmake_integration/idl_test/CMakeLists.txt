# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

project(OpenDDS_car_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
  OpenDDS::Rtps OpenDDS::Rtps_Udp # For run_test.pl --rtps
)

## debugging
set(CMAKE_VERBOSE_MAKEFILE on)

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

# set add opendds_idl compiler option to strip leading directories from #include
# needed for CMake builds with nested idl files
ADD_DDS_IDL_FLAG("-FilenameOnlyIncludes")

# add idl explicitly
ADD_IDL_PATH(./engine/engine_stats)
set(IDL_DEPENDENCIES ./engine/engine_stats/fuel_stats.idl)

# add idl given a path. this updates IDL_DEPENDENCIES.
ADD_IDL_PATH(./engine)
ADD_IDL_FILES_IN_PATH(./engine)

# add idl given a path. this updates IDL_DEPENDENCIES.
ADD_IDL_PATH(./transmission)
# this will import both transmission idl files
ADD_IDL_FILES_IN_PATH(./transmission)

add_library(ecu_lib SHARED)
OPENDDS_TARGET_SOURCES(ecu_lib ecu.idl ${IDL_DEPENDENCIES})

target_link_libraries(ecu_lib ${opendds_libs})

target_link_libraries(${the_ecu} ${opendds_libs} ecu_lib)
