cmake_minimum_required(VERSION 3.3)

project(opendds_messenger_tests)

add_subdirectory(Messenger_1)
add_subdirectory(Messenger_2)
if(OPENDDS_CXX11)
  add_subdirectory(C++11_Messenger)
endif()
