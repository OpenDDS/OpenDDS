cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests)
enable_testing()

set(has_fetch_content FALSE)
if(NOT CMAKE_VERSION VERSION_LESS 3.11)
  set(has_fetch_content TRUE)
  include(FetchContent)
endif()

find_package(OpenDDS REQUIRED)

find_package(GTest QUIET PATHS "${OPENDDS_GTEST}")
if(NOT GTest_FOUND)
  set(gtestsm "${OPENDDS_SOURCE_DIR}/tests/googletest")
  set(gtestsm_cmakelists "${gtestsm}/CMakeLists.txt")
  if(has_fetch_content)
    if(EXISTS "${gtestsm_cmakelists}")
      message("GoogleTest not found, using submodule")
      set(fetch_args SOURCE_DIR "${gtestsm}")
    else()
      message("GoogleTest not found, using clone")
      set(fetch_args
        GIT_REPOSITORY "https://github.com/OpenDDS/googletest"
        GIT_TAG "v1.8.x"
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        USES_TERMINAL_DOWNLOAD TRUE
      )
    endif()
    FetchContent_Declare(googletest ${fetch_args})
  endif()
  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  if(NOT DEFINED gtest_force_shared_crt)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()
  if(NOT DEFINED INSTALL_GTEST)
    # GoogleTest will install by default otherwise
    set(INSTALL_GTEST FALSE CACHE BOOL "" FORCE)
  endif()
  if(has_fetch_content)
    FetchContent_MakeAvailable(googletest)
  elseif(EXISTS "${gtestsm_cmakelists}")
    add_subdirectory("${gtestsm}")
  else()
    message(FATAL_ERROR "GoogleTest not found and can't fetch it")
  endif()
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
get_target_property(_opendds_googletest_bin_dir ${_opendds_googletest} BINARY_DIR)
set(_OPENDDS_GOOGLETEST_DIR "${_opendds_googletest_bin_dir}" CACHE INTERNAL "")

add_subdirectory(cmake)
add_subdirectory(DCPS)
