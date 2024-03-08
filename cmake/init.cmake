# Initializes various variables for paths, features, and options for
# opendds_target_sources.
#
# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

if(_OPENDDS_INIT_CMAKE)
  return()
endif()
set(_OPENDDS_INIT_CMAKE TRUE)

# This is required for CMake <=3.4 for cmake_parse_arguments. Remove this when
# we no longer need to support those versions.
include(CMakeParseArguments)

include("${CMAKE_CURRENT_LIST_DIR}/opendds_version.cmake")

function(_opendds_detect_ace)
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "Getting features from ACE at ${OPENDDS_ACE}")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
  endif()

  find_package(Perl REQUIRED)
  execute_process(
    COMMAND ${PERL_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/detect_ace.pl" "${OPENDDS_ACE}"
    OUTPUT_VARIABLE config_text
    COMMAND_ERROR_IS_FATAL ANY
  )

  foreach(name_value ${config_text})
    if(name_value MATCHES "([^=]+)=([^\n]+)")
      set(name "${CMAKE_MATCH_1}")
      set(value "${CMAKE_MATCH_2}")
      if(OPENDDS_CMAKE_VERBOSE)
        message(STATUS "${name}=${value}")
      endif()
      set("${name}" "${value}" CACHE BOOL "")
    endif()
  endforeach()
endfunction()

include("${CMAKE_CURRENT_LIST_DIR}/config.cmake" OPTIONAL RESULT_VARIABLE OPENDDS_CONFIG_CMAKE)
if(NOT OPENDDS_USE_PREFIX_PATH)
  if(DEFINED OPENDDS_ACE)
    if(NOT EXISTS "${OPENDDS_ACE}")
      message(SEND_ERROR "OPENDDS_ACE (${OPENDDS_ACE}) does not exist")
      return()
    endif()
  else()
    message(SEND_ERROR "OPENDDS_ACE must be defined")
    return()
  endif()
  get_filename_component(OPENDDS_ACE "${OPENDDS_ACE}" ABSOLUTE)

  if(DEFINED OPENDDS_TAO)
    if(NOT EXISTS "${OPENDDS_TAO}")
      message(SEND_ERROR "OPENDDS_TAO (${OPENDDS_TAO}) does not exist")
      return()
    endif()
  elseif(EXISTS "${OPENDDS_ACE}/TAO")
    set(OPENDDS_TAO "${OPENDDS_ACE}/TAO")
  elseif(EXISTS "${OPENDDS_ACE}/../TAO")
    set(OPENDDS_TAO "${OPENDDS_ACE}/../TAO")
  else()
    message(SEND_ERROR
      "OPENDDS_TAO not relative to OPENDDS_ACE (${OPENDDS_ACE}), so OPENDDS_TAO must be defined")
    return()
  endif()
  get_filename_component(OPENDDS_TAO "${OPENDDS_TAO}" ABSOLUTE)
endif()
if(NOT OPENDDS_CONFIG_CMAKE AND NOT ACE_IS_BEING_BUILT)
  _opendds_detect_ace()
endif()

set(_OPENDDS_ALL_FEATURES)
set(_OPENDDS_FEATURE_VARS)
set(_OPENDDS_MPC_FEATURES)
function(_opendds_feature name default_value)
  set(no_value_options MPC MPC_INVERTED CONFIG)
  set(single_value_options MPC_NAME DOC TYPE)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})
  if(NOT DEFINED arg_TYPE)
    set(arg_TYPE BOOL)
  endif()

  string(TOLOWER "${name}" lowercase_name)
  list(APPEND _OPENDDS_ALL_FEATURES "${lowercase_name}")
  set(config_name "OPENDDS_CONFIG_${name}")
  set(name "OPENDDS_${name}")
  set("${name}" "${default_value}" CACHE "${arg_TYPE}" "${arg_DOC}")
  set(value ${${name}})
  list(APPEND _OPENDDS_FEATURE_VARS "${name}")
  if(arg_MPC OR arg_MPC_INVERTED)
    if(NOT DEFINED arg_MPC_NAME)
      set(arg_MPC_NAME "${lowercase_name}")
    endif()
    if(arg_MPC_INVERTED)
      set(mpc_true 0)
      set(mpc_false 1)
    else()
      set(mpc_true 1)
      set(mpc_false 0)
    endif()
    if(value)
      set(mpc_feature "${arg_MPC_NAME}=${mpc_true}")
    else()
      set(mpc_feature "${arg_MPC_NAME}=${mpc_false}")
    endif()
    list(APPEND _OPENDDS_MPC_FEATURES "${mpc_feature}")
  endif()

  if(arg_CONFIG)
    if(arg_TYPE STREQUAL BOOL)
      if(value)
        set(config_value 1)
      else()
        set(config_value 0)
      endif()
    else()
      set(config_value "${value}")
    endif()
    set("${config_name}" "${config_value}" CACHE INTERNAL "" FORCE)
  endif()

  set(_OPENDDS_ALL_FEATURES "${_OPENDDS_ALL_FEATURES}" PARENT_SCOPE)
  set(_OPENDDS_FEATURE_VARS "${_OPENDDS_FEATURE_VARS}" PARENT_SCOPE)
  set(_OPENDDS_MPC_FEATURES "${_OPENDDS_MPC_FEATURES}" PARENT_SCOPE)
endfunction()

# OpenDDS Features
_opendds_feature(BUILT_IN_TOPICS ON CONFIG DOC "Enables built-in-topics (BITs)")
_opendds_feature(OBJECT_MODEL_PROFILE ON CONFIG DOC "Allows using presentation group QoS")
_opendds_feature(PERSISTENCE_PROFILE ON CONFIG
  DOC "Allows using the durability and durability service QoS")
_opendds_feature(OWNERSHIP_PROFILE ON CONFIG
  DOC "Allows history depth QoS and implies OPENDDS_OWNERSHIP_KIND_EXCLUSIVE")
_opendds_feature(OWNERSHIP_KIND_EXCLUSIVE "${OPENDDS_OWNERSHIP_PROFILE}" CONFIG
  DOC "Allows the EXCLUSIVE ownership QoS")
_opendds_feature(CONTENT_SUBSCRIPTION ON CONFIG
  DOC "Implies OPENDDS_CONTENT_FILTERED_TOPIC, OPENDDS_MULTI_TOPIC, and OPENDDS_QUERY_CONDITION")
_opendds_feature(CONTENT_FILTERED_TOPIC "${OPENDDS_CONTENT_SUBSCRIPTION}" CONFIG
  DOC "Allows using ContentFilteredTopic")
_opendds_feature(MULTI_TOPIC "${OPENDDS_CONTENT_SUBSCRIPTION}" CONFIG DOC "Allows using MultiTopic")
_opendds_feature(QUERY_CONDITION "${OPENDDS_CONTENT_SUBSCRIPTION}" CONFIG
  DOC "Allows using QueryCondition")
_opendds_feature(SUPPRESS_ANYS ON CONFIG DOC "Default for opendds_target_sources(SUPPRESS_ANYS)")
_opendds_feature(SECURITY OFF CONFIG DOC "Build with RTPS Security support")
_opendds_feature(SAFETY_PROFILE OFF CONFIG DOC "Build using Safety Profile (Not for CMake-built OpenDDS)")

# ACE Features
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  _opendds_feature(DEBUG ON MPC DOC "(Not for CMake-built OpenDDS)")
  _opendds_feature(OPTIMIZE OFF MPC DOC "(Not for CMake-built OpenDDS)")
else()
  _opendds_feature(DEBUG OFF MPC DOC "(Not for CMake-built OpenDDS)")
  _opendds_feature(OPTIMIZE ON MPC DOC "(Not for CMake-built OpenDDS)")
endif()
_opendds_feature(INLINE ON MPC DOC "(Not for CMake-built OpenDDS)")
if(BUILD_SHARED_LIBS)
  _opendds_feature(STATIC OFF MPC DOC "(Not for CMake-built OpenDDS)")
else()
  _opendds_feature(STATIC ON MPC DOC "(Not for CMake-built OpenDDS)")
endif()
_opendds_feature(XERCES3 "${OPENDDS_SECURITY}" MPC TYPE PATH
  DOC "Build with Xerces XML parser, needed for security and QoS XML Handler")
_opendds_feature(IPV6 OFF MPC DOC "Build with IPv6 support")

# TAO Features
_opendds_feature(TAO_IIOP ON MPC_INVERTED MPC_NAME tao_no_iiop)
_opendds_feature(TAO_OPTIMIZE_COLLOCATED_INVOCATIONS ON MPC)

# Make Sure CMake can use the Paths
file(TO_CMAKE_PATH "${OPENDDS_ACE}" OPENDDS_ACE)
file(TO_CMAKE_PATH "${OPENDDS_MPC}" OPENDDS_MPC)
file(TO_CMAKE_PATH "${OPENDDS_TAO}" OPENDDS_TAO)

option(OPENDDS_CMAKE_VERBOSE "Print verbose output when loading the OpenDDS Config Package" OFF)
if("all" IN_LIST OPENDDS_CMAKE_VERBOSE)
  set(OPENDDS_CMAKE_VERBOSE
    components
    imports
    opendds_target_sources
    CACHE STRING "" FORCE)
endif()
option(OPENDDS_DEFAULT_NESTED "Require topic types to be declared explicitly" ON)
option(OPENDDS_FILENAME_ONLY_INCLUDES "No directory info in generated #includes." OFF)
set(OPENDDS_DEFAULT_SCOPE "PRIVATE" CACHE STRING "Default scope for opendds_target_sources")
set_property(CACHE OPENDDS_DEFAULT_SCOPE PROPERTY STRINGS "PUBLIC" "PRIVATE" "INTERFACE")
option(OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER "Always generate an export header for libraries" OFF)
# This is off because it's not compatible with a possible existing usage of
# target_link_libraries that doesn't specify a scope:
# "All uses of target_link_libraries with a target must be either all-keyword
# or all-plain."
# TODO: Make this default ON in v4.0
option(OPENDDS_AUTO_LINK_DCPS
  "Automatically link dependencies to the target of opendds_target_sources" OFF)
# This is off by default because it could cause "Cannot find source file"
# errors on `TypeSupport.idl` files generated in a another directory.
# TODO: Make this default ON in v4.0
option(OPENDDS_USE_CORRECT_INCLUDE_SCOPE "Include using SCOPE specified in opendds_target_sources" OFF)

macro(_opendds_save_cache name type value)
  list(APPEND _opendds_save_cache_vars ${name})
  set(_opendds_save_cache_${name}_type ${type})
  set(_opendds_save_cache_${name}_value "${${name}}")
  set(${name} "${value}" CACHE ${type} "" FORCE)
endmacro()

macro(_opendds_restore_cache)
  foreach(name ${_opendds_save_cache_vars})
    set(${name} "${_opendds_save_cache_${name}_value}" CACHE
      "${_opendds_save_cache_${name}_type}" "" FORCE)
    unset(_opendds_save_cache_${name}_type)
    unset(_opendds_save_cache_${name}_value)
  endforeach()
  unset(_opendds_save_cache_vars)
endmacro()

function(_opendds_pop_list list_var)
  set(list "${${list_var}}")
  list(LENGTH list len)
  if(len GREATER 0)
    math(EXPR last "${len} - 1")
    list(REMOVE_AT list "${last}")
    set("${list_var}" "${list}" PARENT_SCOPE)
  endif()
endfunction()

function(_opendds_path_list path_list_var)
  if("APPEND" IN_LIST ARGN)
    set(path_list "${${path_list_var}}")
    list(REMOVE_ITEM ARGN APPEND)
  else()
    set(path_list)
  endif()

  if(WIN32)
    set(delimiter ";")
  else()
    set(delimiter ":")
  endif()

  foreach(path ${ARGN})
    if(path_list AND NOT path_list MATCHES "${delimiter}$")
      set(path_list "${path_list}${delimiter}")
    endif()
    set(path_list "${path_list}${path}")
  endforeach()

  set("${path_list_var}" "${path_list}" PARENT_SCOPE)
endfunction()

if(NOT DEFINED OPENDDS_INSTALL_LIB)
  set(OPENDDS_INSTALL_LIB "lib")
endif()

if(OPENDDS_USE_PREFIX_PATH)
  set(_OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
else()
  set(_OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
endif()
get_filename_component(_OPENDDS_ROOT "${_OPENDDS_ROOT}" ABSOLUTE)

if(NOT DEFINED DDS_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(DDS_ROOT "${_OPENDDS_ROOT}/share/dds")
    set(OPENDDS_INCLUDE_DIRS "${_OPENDDS_ROOT}/include")

  else()
    set(DDS_ROOT "${_OPENDDS_ROOT}")
    set(OPENDDS_INCLUDE_DIRS "${_OPENDDS_ROOT}")
  endif()

  if(NOT OPENDDS_IS_BEING_BUILT)
    set(OPENDDS_BIN_DIR "${_OPENDDS_ROOT}/bin")
    set(OPENDDS_LIB_DIR "${_OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")
  endif()
endif()

if(NOT DEFINED ACE_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(ACE_ROOT "${_OPENDDS_ROOT}/share/ace")
    set(ACE_INCLUDE_DIRS "${_OPENDDS_ROOT}/include")
    set(ACE_LIB_DIR "${_OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")

  elseif(OPENDDS_ACE)
    set(ACE_ROOT "${OPENDDS_ACE}")
    set(ACE_INCLUDE_DIRS "${ACE_ROOT}")
    set(ACE_LIB_DIR "${ACE_ROOT}/lib")
  endif()

  set(ACE_BIN_DIR "${ACE_ROOT}/bin")
endif()
_opendds_get_version(OPENDDS_ACE_VERSION ACE "${ACE_ROOT}")
if(NOT DEFINED OPENDDS_CXX_STD)
  if(NOT OPENDDS_ACE_VERSION VERSION_LESS "7.0.0")
    set(OPENDDS_CXX_STD 14)
  elseif(OPENDDS_CXX11)
    set(OPENDDS_CXX_STD 11)
  else()
    set(OPENDDS_CXX_STD 98)
  endif()
endif()

if(NOT DEFINED TAO_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(TAO_ROOT "${_OPENDDS_ROOT}/share/tao")
    set(TAO_INCLUDE_DIR "${_OPENDDS_ROOT}/include")

  elseif(OPENDDS_TAO)
    set(TAO_ROOT "${OPENDDS_TAO}")
    set(TAO_INCLUDE_DIR "${OPENDDS_TAO}")
  endif()

  set(TAO_BIN_DIR "${ACE_BIN_DIR}")
  set(TAO_LIB_DIR "${ACE_LIB_DIR}")
  set(TAO_INCLUDE_DIRS
    "${TAO_INCLUDE_DIR}"
    "${TAO_INCLUDE_DIR}/orbsvcs"
  )
endif()
_opendds_get_version(OPENDDS_TAO_VERSION TAO "${TAO_ROOT}")

message(STATUS "Using OpenDDS ${OPENDDS_VERSION} at ${DDS_ROOT}")
if(DEFINED OPENDDS_MPC)
  message(STATUS "Using MPC at ${OPENDDS_MPC}")
endif()
message(STATUS "Using ACE ${OPENDDS_ACE_VERSION} at ${ACE_ROOT}")
message(STATUS "Using TAO ${OPENDDS_TAO_VERSION} at ${TAO_ROOT}")

if(OPENDDS_STATIC)
  set(OPENDDS_LIBRARY_TYPE STATIC)
else()
  set(OPENDDS_LIBRARY_TYPE SHARED)
endif()

if(OPENDDS_COVERAGE)
  list(APPEND OPENDDS_JUST_OPENDDS_LIBS_INTERFACE_COMPILE_OPTIONS "--coverage")
  list(APPEND OPENDDS_JUST_OPENDDS_LIBS_INTERFACE_LINK_OPTIONS "--coverage")
endif()

if(DEFINED OPENDDS_SANITIZER_COMPILER_ARGS)
  list(APPEND OPENDDS_ALL_LIBS_INTERFACE_COMPILE_OPTIONS "${OPENDDS_SANITIZER_COMPILER_ARGS}")
endif()
if(DEFINED OPENDDS_SANITIZER_LINKER_ARGS)
  list(APPEND OPENDDS_ALL_LIBS_INTERFACE_LINK_OPTIONS "${OPENDDS_SANITIZER_LINKER_ARGS}")
endif()

if(DEFINED OPENDDS_ACE_TAO_HOST_TOOLS)
  set(_OPENDDS_ACE_HOST_TOOLS "${OPENDDS_ACE_TAO_HOST_TOOLS}/bin" CACHE INTERNAL "")
  set(_OPENDDS_TAO_HOST_TOOLS "${OPENDDS_ACE_TAO_HOST_TOOLS}/bin" CACHE INTERNAL "")
endif()
if(DEFINED OPENDDS_HOST_TOOLS)
  set(_OPENDDS_OPENDDS_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/bin" CACHE INTERNAL "")
  if(NOT DEFINED OPENDDS_ACE_TAO_HOST_TOOLS)
    if(IS_DIRECTORY "${OPENDDS_HOST_TOOLS}/ace_tao/bin")
      set(_OPENDDS_ACE_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/ace_tao/bin" CACHE INTERNAL "")
      set(_OPENDDS_TAO_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/ace_tao/bin" CACHE INTERNAL "")
    else()
      set(_OPENDDS_ACE_HOST_TOOLS "${_OPENDDS_OPENDDS_HOST_TOOLS}" CACHE INTERNAL "")
      set(_OPENDDS_TAO_HOST_TOOLS "${_OPENDDS_OPENDDS_HOST_TOOLS}" CACHE INTERNAL "")
    endif()
  endif()
endif()

if(NOT DEFINED OPENDDS_SUPPORTS_SHMEM)
  if(APPLE)
    set(OPENDDS_SUPPORTS_SHMEM FALSE)
  else()
    set(OPENDDS_SUPPORTS_SHMEM TRUE)
  endif()
endif()
