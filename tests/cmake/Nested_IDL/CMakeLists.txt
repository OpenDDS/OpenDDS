# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

project(opendds_nested_idl CXX)
cmake_minimum_required(VERSION 3.8.2)
enable_testing()

find_package(OpenDDS REQUIRED)

set(target_prefix "${PROJECT_NAME}_")

set(OPENDDS_FILENAME_ONLY_INCLUDES ON CACHE BOOL "" FORCE)

set(ecu_lib "${target_prefix}lib")
add_library(${ecu_lib})
set_target_properties(${ecu_lib}
  PROPERTIES OUTPUT_NAME ecu_lib
)
OPENDDS_TARGET_SOURCES(${ecu_lib}
  PUBLIC
    ecu.idl
    ./engine/engine_specs.idl
    ./engine/engine_stats/fuel_stats.idl
    ./transmission/transmission_specs.idl
    ./transmission/transmission_states.idl)
target_link_libraries(${ecu_lib} PUBLIC OpenDDS::Dcps)

set(ecu_exe "${target_prefix}exe")
add_executable(${ecu_exe} "ecu.cpp")
set_target_properties(${ecu_exe}
  PROPERTIES OUTPUT_NAME ecu
)
target_link_libraries(${ecu_exe} OpenDDS::Dcps ${ecu_lib})

# Testing
configure_file("run_test.pl" "." COPYONLY)
add_test(NAME "${target_prefix}test"
  COMMAND perl run_test.pl $<$<BOOL:$<CONFIG>>:-ExeSubDir> $<CONFIG>
)
