cmake_minimum_required(VERSION 3.28...4.0)
project(opendds_tests_dcps_cxx11)
enable_testing()

find_package(OpenDDS REQUIRED)

if(OPENDDS_OWNERSHIP_PROFILE)
  add_subdirectory(Messenger)
endif()
