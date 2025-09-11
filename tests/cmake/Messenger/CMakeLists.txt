cmake_minimum_required(VERSION 3.28...4.0)
project(opendds_messenger_tests)

add_subdirectory(Messenger_1)
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 10.*")
  add_subdirectory(Messenger_Space)
endif()
