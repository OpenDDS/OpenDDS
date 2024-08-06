cmake_minimum_required(VERSION 3.23...3.27)
project(opendds_inspect CXX)

set(dep_libs
  OpenDDS::Rtps_Udp
)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${dep_libs} safety_profile=FALSE)
include(opendds_build_helpers)

add_executable(inspect Inspect.cpp)
_opendds_executable(inspect)
target_link_libraries(inspect PRIVATE ${dep_libs})
