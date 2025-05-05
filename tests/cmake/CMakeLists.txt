cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests_cmake)
enable_testing()

find_package(OpenDDS REQUIRED)

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.2")
  if(OPENDDS_OWNERSHIP_PROFILE)
    add_subdirectory(Messenger)
    add_subdirectory(generated_global)
  endif()
  add_subdirectory(Nested_IDL)
  add_subdirectory(idl_compiler_tests)
  add_subdirectory(ace_tao_only)
endif()

add_subdirectory(include_subdir)

if(NOT OPENDDS_BUILT_USING_CMAKE)
  add_subdirectory(install)
endif()
