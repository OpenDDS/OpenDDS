project(OpenDDS_C++11_Typecode CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)

if(OPENDDS_SAFETY_PROFILE)
  message(FATAL_ERROR "This test cannot be built with Safety Profile enabled")
endif()

if (NOT OPENDDS_CXX11)
  message(FATAL_ERROR "This test requires OpenDDS to be built with C++11 Support")
endif()

set(target_prefix "opendds_cmake_cpp11_typecode_")
set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../DCPS/Compiler/typecode")
set(dst ${CMAKE_CURRENT_BINARY_DIR})

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

# Assert the IDL did not suppress typecodes
if(-Sa IN_LIST OPENDDS_DDS_BASE_IDL_FLAGS OR -St IN_LIST OPENDDS_DDS_BASE_IDL_FLAGS)
  message(FATAL_ERROR "Failed to remove typecode suppression from opendds_idl")
endif()
if(-Sa IN_LIST OPENDDS_TAO_BASE_IDL_FLAGS OR -St IN_LIST OPENDDS_TAO_BASE_IDL_FLAGS)
  message(FATAL_ERROR "Failed to remove typecode suppression from tao_idl")
endif()
