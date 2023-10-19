cmake_minimum_required(VERSION 3.8...3.27)
project(compiler_maps CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

if(NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "This test requires OpenDDS to be built with Ownership Profile enabled")
endif()

# if(NOT OPENDDS_CXX11)
#   message(FATAL_ERROR "This test requires OpenDDS to be built with C++11 Support")
# endif()

set(target_prefix "${PROJECT_NAME}_")
set(src "${CMAKE_CURRENT_SOURCE_DIR}")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(opendds_libs
  OpenDDS::Dcps
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp
)

# IDL Library
set(idl "${target_prefix}idl")
add_library(${idl})
opendds_target_sources(${idl} PUBLIC "${src}/test.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps gtest)
set(idl_dir "${dst}")
set_target_properties(${idl} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${idl_dir}"
  LIBRARY_OUTPUT_DIRECTORY "${idl_dir}"
)

# Assert the mapping used was C++11
get_property(mappings TARGET ${idl} PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
if(NOT ("C++11" IN_LIST mappings))
  message(FATAL_ERROR "${idl}: C++11 NOT in mapping list: ${mappings}")
endif()

set(maps_test "${target_prefix}test")
add_executable(compiler_maps_exe
  "${src}/main.cpp"
)
set_target_properties(compiler_maps_exe PROPERTIES
  # OUTPUT_NAME ${maps_test}
  RUNTIME_OUTPUT_DIRECTORY "${dst}"
)
target_link_libraries(compiler_maps_exe ${opendds_libs} ${idl})

add_test(compiler_maps_test compiler_maps_exe)
