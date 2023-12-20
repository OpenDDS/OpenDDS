cmake_minimum_required(VERSION 3.23...3.27)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/opendds_version.cmake")
project(opendds
  DESCRIPTION
    "OpenDDS is an open-source C++ framework for exchanging data in distributed systems."
  VERSION "${OPENDDS_VERSION}"
  HOMEPAGE_URL "https://opendds.org/"
  LANGUAGES CXX
)
set(CPACK_PACKAGE_CONTACT info@opendds.org)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FetchContent)
include(CPack)

set(OPENDDS_IS_BEING_BUILT TRUE CACHE INTERNAL "")
set(OPENDDS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
if(EXISTS "${OPENDDS_SOURCE_DIR}/dds/DdsDcpsC.h")
  message(FATAL_ERROR "It looks like OpenDDS was already built here using MPC. This could cause "
    "issues with a CMake build. Please clean this source tree or use a separate one.")
endif()
set(OPENDDS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")
get_filename_component(_OPENDDS_SOURCE_DIR_REAL "${OPENDDS_SOURCE_DIR}" REALPATH)
get_filename_component(_OPENDDS_BUILD_DIR_REAL "${OPENDDS_BUILD_DIR}" REALPATH)
if("${_OPENDDS_SOURCE_DIR_REAL}" STREQUAL "${_OPENDDS_BUILD_DIR_REAL}")
  message(FATAL_ERROR "Please build OpenDDS in a different directory from the source tree "
    "directory. Use something like \"cmake -S . -B build\".")
endif()
set(OPENDDS_BIN_DIR "${OPENDDS_BUILD_DIR}/bin" CACHE INTERNAL "")
set(OPENDDS_LIB_DIR "${OPENDDS_BUILD_DIR}/lib" CACHE INTERNAL "")
list(APPEND CMAKE_PREFIX_PATH "${OPENDDS_SOURCE_DIR}")
list(APPEND CMAKE_FIND_ROOT_PATH "${OPENDDS_SOURCE_DIR}")
set(_OPENDDS_CMAKE_DIR "${OPENDDS_SOURCE_DIR}/cmake" CACHE INTERNAL "")

# Get and build ACE/TAO if OPENDDS_ACE wasn't defined and initialize the core
# of the CMake support in init.cmake.
if(NOT DEFINED OPENDDS_ACE OR DEFINED ACE_IS_BEING_BUILT)
  # This is before init.cmake so init.cmake can get the features from ACE if
  # ACE was already built.
  include("${_OPENDDS_CMAKE_DIR}/get_ace_tao.cmake")
endif()
include("${_OPENDDS_CMAKE_DIR}/init.cmake")
if(DEFINED ACE_IS_BEING_BUILT)
  # This is after init.cmake so ACE/TAO can be configured using the features
  # from init.cmake.
  include("${_OPENDDS_CMAKE_DIR}/build_ace_tao.cmake")
endif()

if(NOT DEFINED OPENDDS_RAPIDJSON)
  set(OPENDDS_RAPIDJSON "${OPENDDS_SOURCE_DIR}/tools/rapidjson")
  if(NOT EXISTS "${OPENDDS_RAPIDJSON}/include")
    FetchContent_Declare(rapidjson
      GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
      GIT_TAG fd3dc29a5c2852df569e1ea81dbde2c412ac5051
    )
    FetchContent_Populate(rapidjson)
    set(OPENDDS_RAPIDJSON "${rapidjson_SOURCE_DIR}")
  endif()
  set(OPENDDS_INSTALL_RAPIDJSON TRUE)
endif()

if(NOT CMAKE_CROSSCOMPILING)
  add_subdirectory(dds/DCPS) # OpenDDS_Util
  add_subdirectory(dds/idl) # opendds_idl
  if(OPENDDS_JUST_BUILD_HOST_TOOLS)
    return()
  endif()
endif()

add_subdirectory(dds) # OpenDDS_Dcps
add_subdirectory(dds/DCPS/transport/multicast)
add_subdirectory(dds/DCPS/RTPS)
add_subdirectory(dds/DCPS/transport/rtps_udp)
add_subdirectory(dds/DCPS/transport/shmem)
add_subdirectory(dds/DCPS/transport/tcp)
add_subdirectory(dds/DCPS/InfoRepoDiscovery)
add_subdirectory(dds/DCPS/transport/udp)
add_subdirectory(dds/InfoRepo)
if(OPENDDS_XERCES3)
  add_subdirectory(dds/DCPS/QOS_XML_Handler)
endif()
if(OPENDDS_SECURITY)
  add_subdirectory(dds/DCPS/security)
  if(OPENDDS_BUILT_IN_TOPICS)
    add_subdirectory(tools/dds/rtpsrelaylib)
    add_subdirectory(tools/rtpsrelay)
  endif()
endif()
add_subdirectory(tools/inspect)

# Installation
set(cmake_dest "${CMAKE_INSTALL_DATAROOTDIR}/cmake/OpenDDS")
install(EXPORT opendds_targets
  FILE "opendds_targets.cmake"
  DESTINATION ${cmake_dest}
)
set(config_cmake "${CMAKE_CURRENT_BINARY_DIR}/config.cmake")
set(opendds_feature_vars)
foreach(var ${_OPENDDS_FEATURE_VARS})
  set(opendds_feature_vars "${opendds_feature_vars}set(${var} ${${var}})\n")
endforeach()
configure_file("${_OPENDDS_CMAKE_DIR}/config.cmake.in" "${config_cmake}")
install(
  FILES
    cmake/OpenDDSConfig.cmake
    cmake/OpenDDSConfigVersion.cmake
    cmake/ace_group.cmake
    cmake/dds_idl_sources.cmake
    cmake/export.h.in
    cmake/import_common.cmake
    cmake/init.cmake
    cmake/opendds_group.cmake
    cmake/opendds_target_sources.cmake
    cmake/opendds_testing.cmake
    cmake/opendds_version.cmake
    cmake/tao_group.cmake
    cmake/tao_idl_sources.cmake
    "${config_cmake}"
  DESTINATION "${cmake_dest}"
)
install(FILES VERSION.txt DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/dds/")
if(OPENDDS_INSTALL_RAPIDJSON)
  install(DIRECTORY "${OPENDDS_RAPIDJSON}/include/rapidjson"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# Examples and Tests
if(BUILD_TESTING)
  set(_OPENDDS_BUILD_TESTS_DEFAULT TRUE)
else()
  set(_OPENDDS_BUILD_TESTS_DEFAULT FALSE)
endif()
set(OPENDDS_BUILD_TESTS ${_OPENDDS_BUILD_TESTS_DEFAULT} CACHE BOOL "Build OpenDDS Tests")
set(OPENDDS_BUILD_EXAMPLES TRUE CACHE BOOL "Build OpenDDS Examples")
if(OPENDDS_BUILD_EXAMPLES OR OPENDDS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(DevGuideExamples/DCPS/Messenger)
endif()
if(OPENDDS_BUILD_TESTS)
  add_subdirectory(tests)
endif()
