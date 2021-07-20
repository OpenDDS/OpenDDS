cmake_minimum_required(VERSION 3.3)

project(opendds_cmake_tests_root_project)
enable_testing()

find_package(OpenDDS REQUIRED)

configure_file("CTestCustom.cmake" "." COPYONLY)

add_subdirectory(
  "../../DevGuideExamples/DCPS/Messenger"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../DevGuideExamples/DCPS/Messenger/build")

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.2")
  if(OPENDDS_OWNERSHIP_PROFILE)
    add_subdirectory(Messenger)
    if(NOT OPENDDS_STATIC)
      add_subdirectory(generated_global)
    endif()
  endif()
  if(NOT OPENDDS_SAFETY_PROFILE AND OPENDDS_CXX11)
      add_subdirectory(C++11_typecode)
  endif()
  add_subdirectory(Nested_IDL)
endif()
