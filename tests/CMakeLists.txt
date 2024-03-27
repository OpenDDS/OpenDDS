cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_tests)
enable_testing()

find_package(OpenDDS REQUIRED)

find_package(GTest QUIET PATHS "${OPENDDS_GTEST}")
if(NOT GTest_FOUND)
  set(gtestsm "${OPENDDS_SOURCE_DIR}/tests/googletest")
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
  FetchContent_MakeAvailable(googletest)
endif()
if(TARGET GTest::gtest)
  get_target_property(google_test_bin_dir GTest::gtest BINARY_DIR)
  set(_OPENDDS_GOOGLE_TEST_DIR "${google_test_bin_dir}" CACHE INTERNAL "")
elseif(TARGET gtest)
  get_target_property(google_test_bin_dir gtest BINARY_DIR)
  set(_OPENDDS_GOOGLE_TEST_DIR "${google_test_bin_dir}" CACHE INTERNAL "")
endif()

# TODO: Replace with:
# add_subdirectory(cmake)
add_subdirectory(cmake/ace_tao_only)
if(OPENDDS_OWNERSHIP_PROFILE)
  add_subdirectory(cmake/Messenger)
endif()
add_subdirectory(cmake/idl_compiler_tests)
add_subdirectory(cmake/include_subdir)
# TODO: This test always builds shared libraries and linker complains about
# ACE/TAO libs lacking -fPIC when ACE is static.
if(OPENDDS_OWNERSHIP_PROFILE AND NOT OPENDDS_STATIC)
  add_subdirectory(cmake/generated_global)
endif()

add_subdirectory(DCPS)
