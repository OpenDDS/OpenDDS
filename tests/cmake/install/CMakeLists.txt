cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_install_test)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

include(ExternalProject)

set(INSTALL_IMPORTED_RUNTIME_ARTIFACTS TRUE)
if(${CMAKE_VERSION} VERSION_LESS "3.21.0" OR OPENDDS_STATIC)
  set(INSTALL_IMPORTED_RUNTIME_ARTIFACTS FALSE)
endif()

set(dest_install_prefix "${CMAKE_CURRENT_BINARY_DIR}/the-install-prefix")
set(common_cmake_args
  "-DCMAKE_INSTALL_PREFIX:PATH=${dest_install_prefix}"
  "-DCMAKE_PREFIX_PATH:PATH=${_OPENDDS_ROOT}$<SEMICOLON>${dest_install_prefix}"
  "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
)
if(DEFINED CMAKE_CXX_STANDARD)
  list(APPEND common_cmake_args "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}")
endif()
if(DEFINED CMAKE_TOOLCHAIN_FILE)
  list(APPEND common_cmake_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

set(lib "${PROJECT_NAME}_lib")
ExternalProject_Add(
  ${lib}
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library"
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}/libproj"
  CMAKE_ARGS
    "-DINSTALL_IMPORTED_RUNTIME_ARTIFACTS=${INSTALL_IMPORTED_RUNTIME_ARTIFACTS}"
    "${common_cmake_args}"
)

set(user "${PROJECT_NAME}_user")
ExternalProject_Add(
  ${user}
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/user"
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}/userproj"
  CMAKE_ARGS "${common_cmake_args}"
  DEPENDS ${lib}
)

# Test in current environment
opendds_add_test(NAME current_env)

if(UNIX AND INSTALL_IMPORTED_RUNTIME_ARTIFACTS)
  # IMPORTED_RUNTIME_ARTIFACTS was introduced in CMake 3.21, and to test it we
  # need to create a clean environment.
  configure_file("run_in_prefix.sh" "." COPYONLY)
  execute_process(
    COMMAND perl
      "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/scripts/install_test_framework.pl"
        "${dest_install_prefix}"
    RESULT_VARIABLE rc
  )
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "install_test_framework.pl failed: ${rc}")
  endif()

  opendds_add_test(NAME clean_env ARGS clean-env)
endif()
