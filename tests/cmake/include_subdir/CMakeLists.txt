cmake_minimum_required(VERSION 3.3...3.27)
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
project(opendds_include_subdir CXX)

find_package(OpenDDS REQUIRED)

add_library(${PROJECT_NAME})
opendds_target_sources(${PROJECT_NAME}
  "subdir/a.idl"
  "subdir/b.idl"
  OPENDDS_IDL_OPTIONS "-Isubdir"
)
target_link_libraries(${PROJECT_NAME} OpenDDS::Dcps)
