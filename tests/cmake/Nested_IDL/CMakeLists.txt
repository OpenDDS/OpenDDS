# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_nested_idl CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")

_opendds_save_cache(OPENDDS_FILENAME_ONLY_INCLUDES BOOL ON)

set(ecu_lib "${target_prefix}lib")
add_library(${ecu_lib})
set_target_properties(${ecu_lib}
  PROPERTIES OUTPUT_NAME ecu_lib
)
if(OPENDDS_FILENAME_ONLY_INCLUDES)
  set(ots_args
    TAO_IDL_OPTIONS -DOPENDDS_FILENAME_ONLY_INCLUDES
    OPENDDS_IDL_OPTIONS -DOPENDDS_FILENAME_ONLY_INCLUDES
  )
else()
  set(ots_args
    samename.idl
    engine/samename.idl
    engine/engine_stats/samename.idl
  )
endif()
opendds_target_sources(${ecu_lib}
  PUBLIC
    ecu.idl
    engine/engine_specs.idl
    engine/engine_stats/fuel_stats.idl
    transmission/transmission_specs.idl
    transmission/transmission_states.idl
  ${ots_args}
)
target_link_libraries(${ecu_lib} PUBLIC OpenDDS::Dcps)

set(ecu_exe "${target_prefix}exe")
add_executable(${ecu_exe} "ecu.cpp")
set_target_properties(${ecu_exe}
  PROPERTIES OUTPUT_NAME ecu
)
target_link_libraries(${ecu_exe} OpenDDS::Dcps ${ecu_lib})

# Testing
opendds_add_test()

_opendds_restore_cache()
