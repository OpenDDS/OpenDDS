cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_include_subdir CXX)

find_package(OpenDDS REQUIRED)

add_library(${PROJECT_NAME})
opendds_target_sources(${PROJECT_NAME}
  "subdir/a.idl"
  "subdir/b.idl"
  OPENDDS_IDL_OPTIONS "-Isubdir"
)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Dcps)
