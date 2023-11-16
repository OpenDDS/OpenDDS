cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_compiler_optional CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

set(target_prefix "${PROJECT_NAME}_")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(opendds_libs
  OpenDDS::Dcps
)

# IDL Library
set(idl "${target_prefix}idl")
add_library(${idl})
opendds_target_sources(${idl} PUBLIC "optional.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
set(idl_dir "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(opendds_compiler_optional_exe
  "main.cpp"
)
set_target_properties(opendds_compiler_optional_exe PROPERTIES
  OUTPUT_NAME optional
)
target_link_libraries(opendds_compiler_optional_exe ${opendds_libs} ${idl})

configure_file(run_test.pl . COPYONLY)
opendds_add_test(COMMAND perl run_test.pl cpp11 EXTRA_LIB_DIRS "${idl_dir}")
