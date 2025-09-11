cmake_minimum_required(VERSION 3.28...4.0)
project(opendds_examples_dcps)
enable_testing()

find_package(OpenDDS REQUIRED)

if(OPENDDS_QT AND NOT OPENDDS_CXX_STD_YEAR LESS 2011)
  add_subdirectory(ishapes)
endif()
