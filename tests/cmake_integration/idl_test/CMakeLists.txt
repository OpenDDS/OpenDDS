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

set(thecar "car")

set(src "${CMAKE_CURRENT_BINARY_DIR}/..")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

foreach(file
  car.cpp car.idl)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

file(MAKE_DIRECTORY ${dst}/engine)
configure_file(${src}/engine/engine_specs.idl ${dst}/engine COPYONLY)

file(MAKE_DIRECTORY ${dst}/transmission)
configure_file(${src}/transmission/transmission_specs.idl ${dst}/transmission COPYONLY)

add_executable(${thecar} "car.cpp")

# add idl explicitly
ADD_IDL_PATH(./engine)
set(IDL_DEPENDENCIES engine/engine_specs.idl)

# add idl given a path
Add_IDL_PATH(./transmission)
ADD_IDL_FILES_IN_PATH(./transmission)

add_library(car_lib SHARED)
OPENDDS_TARGET_SOURCES(car_lib car.idl ${IDL_DEPENDENCIES})

target_link_libraries(car_lib ${opendds_libs})

target_link_libraries(${thecar} ${opendds_libs} car_lib)
