cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests_dcps_compiler_no_init_before_deserialize_cpp11 CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

# IDL Library
set(idl "${target_prefix}idl")
add_library(${idl})
opendds_target_sources(${idl} PUBLIC "../no_init_before_deserialize.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
target_compile_definitions(${idl} PUBLIC CPP11_MAPPING=1)

# Executable
set(exe "${target_prefix}exe")
add_executable(${exe} "../no_init_before_deserialize.cpp")
set_target_properties(${exe} PROPERTIES
  OUTPUT_NAME no_init_before_deserialize_cpp11
)
target_link_libraries(${exe} OpenDDS::Dcps ${idl})

opendds_add_test(ARGS classic RUN_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
