cmake_minimum_required(VERSION 3.3)

project(opendds_cmake_tests_root_project)

find_package(OpenDDS REQUIRED)

add_subdirectory(Messenger)
add_subdirectory(Nested_IDL)
if(NOT OPENDDS_STATIC)
  add_subdirectory(generated_global)
endif()
if(NOT OPENDDS_SAFETY_PROFILE AND OPENDDS_CXX11)
  add_subdirectory(C++11_typecode)
endif()
