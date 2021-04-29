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
OPENDDS_TARGET_SOURCES(${idl} "${src}/typecode.idl" OPENDDS_IDL_OPTIONS "-Lc++11" OPENDDS_SUPPRESS_TYPECODES OFF)
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
set_target_properties(${idl} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Idl"
)

# main
set(main "${target_prefix}main")
add_executable(${main}
  "${dst}/../main.cpp"
)
set_target_properties(${main} PROPERTIES
  OUTPUT_NAME "main"
  RUNTIME_OUTPUT_DIRECTORY "${dst}/main"
)
target_link_libraries(${main} ${idl})

# Assert the mapping used was C++11
get_property(mappings TARGET ${idl} PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
if(NOT ("C++11" IN_LIST mappings))
  message(FATAL_ERROR "${idl}: C++11 NOT in mapping list: ${mappings}")
endif()
