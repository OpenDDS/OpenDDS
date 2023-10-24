cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_compiler_char_literals CXX)
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
opendds_target_sources(${idl} PUBLIC "test.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
set(idl_dir "${dst}/cpp11")
set_target_properties(${idl} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${idl_dir}"
  LIBRARY_OUTPUT_DIRECTORY "${idl_dir}"
)

add_executable(opendds_compiler_char_literals_exe
  "main.cpp"
)
set_target_properties(opendds_compiler_char_literals_exe PROPERTIES
  OUTPUT_NAME char-literals-cpp11
  RUNTIME_OUTPUT_DIRECTORY "${dst}/cpp11"
)
target_link_libraries(opendds_compiler_char_literals_exe ${opendds_libs} ${idl} gtest)

configure_file(run_test.pl . COPYONLY)
opendds_add_test(COMMAND perl run_test.pl cpp11 EXTRA_LIB_DIRS "${idl_dir}")
