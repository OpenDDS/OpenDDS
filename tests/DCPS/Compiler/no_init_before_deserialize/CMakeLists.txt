cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests_dcps_compiler_no_init_before_deserialize CXX)
enable_testing()

find_package(OpenDDS REQUIRED)

add_subdirectory(classic)
if(NOT OPENDDS_CXX_STD_YEAR LESS 2011)
  add_subdirectory(cpp11)
endif()
