cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests_dcps_c++11)
enable_testing()

find_package(OpenDDS REQUIRED)

if(OPENDDS_OWNERSHIP_PROFILE)
  add_subdirectory(Messenger)
endif()
