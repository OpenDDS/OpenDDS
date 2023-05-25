# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

if(_OPENDDS_INIT_CMAKE)
  return()
endif()
set(_OPENDDS_INIT_CMAKE TRUE)

find_package(Perl REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/config.cmake")

set(_OPENDDS_ALL_FEATURES)
function(_opendds_feature name default_value)
  string(TOLOWER "${name}" lowercase_name)
  set(_OPENDDS_ALL_FEATURES ${_OPENDDS_ALL_FEATURES} "${lowercase_name}" PARENT_SCOPE)
  set(name "OPENDDS_${name}")
  if(NOT DEFINED "${name}")
    set("${name}" "${default_value}" PARENT_SCOPE)
  endif()
endfunction()

_opendds_feature(DEBUG ON)
_opendds_feature(INLINE ON)
_opendds_feature(STATIC OFF)
_opendds_feature(TAO_IIOP ON)
_opendds_feature(TAO_OPTIMIZE_COLLOCATED_INVOCATIONS ON)
_opendds_feature(BUILT_IN_TOPICS ON)
_opendds_feature(OBJECT_MODEL_PROFILE ON)
_opendds_feature(PERSISTENCE_PROFILE ON)
_opendds_feature(OWNERSHIP_PROFILE ON)
_opendds_feature(OWNERSHIP_KIND_EXCLUSIVE ${OPENDDS_OWNERSHIP_PROFILE})
_opendds_feature(CONTENT_SUBSCRIPTION ON)
_opendds_feature(CONTENT_FILTERED_TOPIC ${OPENDDS_CONTENT_SUBSCRIPTION})
_opendds_feature(MULTI_TOPIC ${OPENDDS_CONTENT_SUBSCRIPTION})
_opendds_feature(QUERY_CONDITION ${OPENDDS_CONTENT_SUBSCRIPTION})
_opendds_feature(SUPPRESS_ANYS ON)
_opendds_feature(SECURITY OFF)
_opendds_feature(XERCES3 ${OPENDDS_SECURITY})
_opendds_feature(SAFETY_PROFILE OFF)

# Make Sure CMake can use the Paths
file(TO_CMAKE_PATH "${OPENDDS_ACE}" OPENDDS_ACE)
file(TO_CMAKE_PATH "${OPENDDS_MPC}" OPENDDS_MPC)
file(TO_CMAKE_PATH "${OPENDDS_TAO}" OPENDDS_TAO)

option(OPENDDS_CMAKE_VERBOSE "Print verbose output when loading the OpenDDS Config Package" OFF)
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

if(NOT DEFINED OPENDDS_INSTALL_LIB)
  set(OPENDDS_INSTALL_LIB "lib")
endif()

if(OPENDDS_USE_PREFIX_PATH)
  set(OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
else()
  set(OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
endif()
get_filename_component(OPENDDS_ROOT "${OPENDDS_ROOT}" ABSOLUTE)

if(NOT DEFINED DDS_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(DDS_ROOT "${OPENDDS_ROOT}/share/dds")
    set(OPENDDS_INCLUDE_DIRS "${OPENDDS_ROOT}/include")

  else()
    set(DDS_ROOT "${OPENDDS_ROOT}")
    set(OPENDDS_INCLUDE_DIRS "${OPENDDS_ROOT}")
  endif()

  set(OPENDDS_BIN_DIR "${OPENDDS_ROOT}/bin")
  set(OPENDDS_LIB_DIR "${OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")
endif()

if(NOT DEFINED ACE_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(ACE_ROOT "${OPENDDS_ROOT}/share/ace")
    set(ACE_INCLUDE_DIRS "${OPENDDS_ROOT}/include")
    set(ACE_LIB_DIR "${OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")

  elseif(OPENDDS_ACE)
    set(ACE_ROOT ${OPENDDS_ACE})
    set(ACE_INCLUDE_DIRS "${ACE_ROOT}")
    set(ACE_LIB_DIR "${ACE_ROOT}/lib")

  else()
    message(SEND_ERROR "Failed to locate ACE")
    return()
  endif()

  set(ACE_BIN_DIR "${ACE_ROOT}/bin")
endif()

if(NOT DEFINED TAO_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(TAO_ROOT "${OPENDDS_ROOT}/share/tao")
    set(TAO_INCLUDE_DIR "${OPENDDS_ROOT}/include")

  elseif(OPENDDS_TAO)
    set(TAO_ROOT "${OPENDDS_TAO}")
    set(TAO_INCLUDE_DIR "${OPENDDS_TAO}")

  else()
    message(SEND_ERROR "Failed to locate TAO")
    return()
  endif()

  set(TAO_BIN_DIR "${ACE_BIN_DIR}")
  set(TAO_LIB_DIR "${ACE_LIB_DIR}")
  set(TAO_INCLUDE_DIRS
    "${TAO_INCLUDE_DIR}"
    "${TAO_INCLUDE_DIR}/orbsvcs"
  )
endif()

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
