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

# Test in current environment
add_test(NAME "${PROJECT_NAME}_current_env"
  COMMAND perl "${CMAKE_CURRENT_SOURCE_DIR}/run_test.pl"
  WORKING_DIRECTORY "${prefix}"
)

if(UNIX AND NOT ${CMAKE_VERSION} VERSION_LESS "3.21.0")
  # IMPORTED_RUNTIME_ARTIFACTS was introduced in CMake 3.21, and to test it we
  # need to create a clean environment.
  execute_process(
    COMMAND perl "${CMAKE_CURRENT_SOURCE_DIR}/install_test_framework.pl" "${prefix}"
    RESULT_VARIABLE rc
  )
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "install_test_framework.pl failed: ${rc}")
  endif()
  add_test(NAME "${PROJECT_NAME}_clean_env"
    # We need to run this in a clean environment to make sure it's just using
    # what's in the prefix. env -i omits all the current environment variables,
    # sh sets basic variables, and run_in_prefix.sh sets the variables for
    # using the prefix.
    COMMAND env -i sh "${CMAKE_CURRENT_SOURCE_DIR}/run_in_prefix.sh" "${prefix}"
      perl "${CMAKE_CURRENT_SOURCE_DIR}/run_test.pl"
    WORKING_DIRECTORY "${prefix}"
  )
else()
endif()
