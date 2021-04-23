project(OpenDDS_C++11_Typecode CXX)
cmake_minimum_required(VERSION 3.8.2)

if(NOT OpenDDS_FOUND)
  find_package(OpenDDS REQUIRED)
endif()

if(OPENDDS_NO_OPENDDS_SAFETY_PROFILE EQUAL OFF)
  message(FATAL_ERROR "This test cannot be built with Safety Profile enabled")
endif()

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})
OPENDDS_NO_SUPPRESS_ANYS()
set(target_prefix "opendds_cmake_cpp11_typecode_")
set(src "${CMAKE_CURRENT_SOURCE_DIR}")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(opendds_libs
  OpenDDS::Dcps
)
# IDL Library
set(idl "${target_prefix}idl")
add_library(${idl})
OPENDDS_TARGET_SOURCES(${idl} "${src}/typecode.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
set_target_properties(${idl} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Idl"
)

# Assert the mapping used was C++11
get_property(mappings TARGET ${idl} PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
if(NOT ("C++11" IN_LIST mappings))
  message(FATAL_ERROR "${idl}: C++11 NOT in mapping list: ${mappings}")
endif()
