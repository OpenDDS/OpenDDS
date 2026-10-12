cmake_minimum_required(VERSION 3.8...4.0)
project(opendds_compiler_maps_classic CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")

set(idl "${target_prefix}idl")
add_library(${idl})
opendds_target_sources(${idl}
  PUBLIC
    test.idl
  TAO_IDL_OPTIONS -St
)
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)

set(exe "${target_prefix}exe")
add_executable(${exe} main.cpp)
set_target_properties(${exe} PROPERTIES OUTPUT_NAME maps_classic)
target_link_libraries(${exe} OpenDDS::Dcps ${idl})

opendds_add_test(
  NAME roundtrip
  COMMAND $<TARGET_FILE:${exe}>
  EXTRA_LIB_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
)
