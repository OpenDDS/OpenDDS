cmake_minimum_required(VERSION 3.28...4.0)
project(opendds_tests)
enable_testing()

include(FetchContent)

find_package(OpenDDS REQUIRED)

find_package(GTest QUIET PATHS "${OPENDDS_GTEST}")
if(NOT GTest_FOUND)
  set(gtestsm "${OPENDDS_SOURCE_DIR}/tests/googletest")
  set(gtestsm_cmakelists "${gtestsm}/CMakeLists.txt")
  if(EXISTS "${gtestsm_cmakelists}")
    message("GoogleTest not found, using submodule")
    set(fetch_args SOURCE_DIR "${gtestsm}")
  else()
    message("GoogleTest not found, using clone")
    set(fetch_args
      GIT_REPOSITORY "https://github.com/google/googletest"
      GIT_TAG "v1.17.x"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
      USES_TERMINAL_DOWNLOAD TRUE
    )
  endif()
  FetchContent_Declare(googletest ${fetch_args})
  if(NOT DEFINED INSTALL_GTEST)
    # GoogleTest will install by default otherwise
    set(INSTALL_GTEST FALSE CACHE BOOL "" FORCE)
  endif()
  FetchContent_MakeAvailable(googletest)
endif()
if(TARGET GTest::gtest)
  set(_opendds_googletest GTest::gtest CACHE INTERNAL "" FORCE)
elseif(TARGET gtest)
  set(_opendds_googletest gtest CACHE INTERNAL "" FORCE)
else()
  message(FATAL_ERROR "No GoogleTest target found!")
endif()
add_library(OpenDDS_GoogleTest INTERFACE)
target_link_libraries(OpenDDS_GoogleTest INTERFACE ${_opendds_googletest})
get_target_property(_opendds_googletest_type ${_opendds_googletest} TYPE)
if(_opendds_googletest_type STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(OpenDDS_GoogleTest INTERFACE GTEST_LINKED_AS_SHARED_LIBRARY=1)
endif()
get_target_property(_opendds_googletest_bin_dir ${_opendds_googletest} RUNTIME_OUTPUT_DIRECTORY)
set(_OPENDDS_GOOGLETEST_DIR "${_opendds_googletest_bin_dir}" CACHE INTERNAL "")

add_subdirectory(cmake)
add_subdirectory(DCPS)
