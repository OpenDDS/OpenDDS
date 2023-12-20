cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_tests)
enable_testing()

find_package(OpenDDS REQUIRED)

find_package(GTest QUIET PATHS "${OPENDDS_GTEST}")
if(NOT GTest_FOUND)
  set(gtestsm "${CMAKE_CURRENT_LIST_DIR}/googletest")
  if(EXISTS "${gtestsm}/CMakeLists.txt")
    message("GTest not found, using submodule")
    set(fetch_args SOURCE_DIR "${gtestsm}")
  else()
    message("GTest not found, using clone")
    set(fetch_args
      GIT_REPOSITORY "https://github.com/OpenDDS/googletest"
      GIT_TAG "v1.8.x"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
      USES_TERMINAL_DOWNLOAD TRUE
    )
  endif()
  FetchContent_Declare(googletest ${fetch_args})
  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

if(TARGET GTest::gtest)
  set(_opendds_googletest GTest::gtest CACHE INTERNAL "" FORCE)
elseif(TARGET gtest)
  set(_opendds_googletest gtest CACHE INTERNAL "" FORCE)
endif()

add_library(OpenDDS_GoogleTest INTERFACE)
target_link_libraries(OpenDDS_GoogleTest INTERFACE ${_opendds_googletest})
get_target_property(_opendds_googletest_type ${_opendds_googletest} TYPE)
if(_opendds_googletest_type STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(OpenDDS_GoogleTest INTERFACE GTEST_LINKED_AS_SHARED_LIBRARY=1)
endif()

get_target_property(_opendds_googletest_bin_dir ${_opendds_googletest} BINARY_DIR)
set(_OPENDDS_GOOGLETEST_DIR "${_opendds_googletest_bin_dir}" CACHE INTERNAL "")

add_subdirectory(cmake)
add_subdirectory(DCPS)
add_subdirectory(unit-tests)
