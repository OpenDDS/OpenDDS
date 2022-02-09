cmake_minimum_required(VERSION 3.3)

project(opendds_cmake_install_test)
enable_testing()

find_package(OpenDDS REQUIRED)

include(ExternalProject)

set(prefix "${CMAKE_CURRENT_BINARY_DIR}/the-install-prefix")

set(lib "${PROJECT_NAME}_library")
ExternalProject_Add(
  ${lib}
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library"
  CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:PATH=${prefix}"
)

set(user "${PROJECT_NAME}_user")
ExternalProject_Add(
  ${user}
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/user"
  CMAKE_ARGS
    "-DCMAKE_PREFIX_PATH:PATH=${prefix}"
    "-DCMAKE_INSTALL_PREFIX:PATH=${prefix}"
  DEPENDS ${lib}
)

# Testing
configure_file("run_test.pl" "." COPYONLY)

# Test in current environment
add_test(NAME "${PROJECT_NAME}_current_env"
  COMMAND perl run_test.pl
)

if(UNIX AND NOT ${CMAKE_VERSION} VERSION_LESS "3.21.0")
  # IMPORTED_RUNTIME_ARTIFACTS was introduced in CMake 3.21, and to test it we
  # need to create a clean environment.
  configure_file("run_in_prefix.sh" "." COPYONLY)
  execute_process(
    COMMAND perl "${CMAKE_CURRENT_SOURCE_DIR}/install_test_framework.pl" "${prefix}"
    RESULT_VARIABLE rc
  )
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "install_test_framework.pl failed: ${rc}")
  endif()
  add_test(NAME "${PROJECT_NAME}_clean_env"
    COMMAND perl run_test.pl clean-env
  )
endif()
