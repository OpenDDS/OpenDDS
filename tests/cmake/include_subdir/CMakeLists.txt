project(opendds_include_subdir CXX)
cmake_minimum_required(VERSION 3.3)

find_package(OpenDDS REQUIRED)

add_library(${PROJECT_NAME})
OPENDDS_TARGET_SOURCES(${PROJECT_NAME}
  "subdir/a.idl"
  "subdir/b.idl"
  OPENDDS_IDL_OPTIONS "-Isubdir"
)
target_link_libraries(${PROJECT_NAME} OpenDDS::Dcps)
