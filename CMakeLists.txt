# NOTE: This is NOT a CMakeLists file for OpenDDS itself, OpenDDS must be
# configured using the configure script. This is one for building all examples
# and tests that are built using CMake.

cmake_minimum_required(VERSION 3.3)

project(opendds_root_project)

add_subdirectory("DevGuideExamples/DCPS")
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.8.2)
  add_subdirectory("tests/cmake_integration")
endif()
