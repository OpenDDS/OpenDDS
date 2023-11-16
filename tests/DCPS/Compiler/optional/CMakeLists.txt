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

set(optional "${target_prefix}exe")
add_executable(${optional}
  "main.cpp"
)
set_target_properties(${optional} PROPERTIES
  OUTPUT_NAME optional_annotation
  RUNTIME_OUTPUT_DIRECTORY "${dst}"
)
target_link_libraries(${optional} ${opendds_libs} ${idl})

configure_file(run_test.pl . COPYONLY)
opendds_add_test(COMMAND perl run_test.pl cpp11 EXTRA_LIB_DIRS "${idl_dir}")
