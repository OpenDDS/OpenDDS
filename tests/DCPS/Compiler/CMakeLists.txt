cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_tests_dcps_compiler)
enable_testing()

find_package(OpenDDS REQUIRED)

if(NOT OPENDDS_CXX_STD_YEAR LESS 2011)
  add_subdirectory(char_literals)
endif()
