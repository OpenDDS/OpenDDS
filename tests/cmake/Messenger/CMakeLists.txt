cmake_minimum_required(VERSION 3.3)

project(opendds_messenger_tests)

add_subdirectory(Messenger_1)
add_subdirectory(Messenger_2)
if (NOT CMAKE_GENERATOR MATCHES "Visual Studio 10.*")
  add_subdirectory(Messenger_Space)
endif()
if(OPENDDS_CXX11)
  add_subdirectory(C++11_Messenger)
endif()
