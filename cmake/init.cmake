# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

if(_OPENDDS_INIT_CMAKE)
  return()
endif()
set(_OPENDDS_INIT_CMAKE TRUE)

if(NOT DEFINED _OPENDDS_CMAKE_DIR)
  set(_OPENDDS_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
endif()
if(NOT "${_OPENDDS_CMAKE_DIR}" IN_LIST CMAKE_MODULE_PATH)
  list(APPEND CMAKE_MODULE_PATH "${_OPENDDS_CMAKE_DIR}")
endif()

find_package(Perl)

function(_opendds_detect_ace)
  if(_OPENDDS_DETECTED_ACE)
    return()
  endif()
  set(_OPENDDS_DETECTED_ACE TRUE CACHE INTERNAL "")

  execute_process(
    COMMAND ${PERL_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/detect_ace.pl" "${OPENDDS_ACE}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE config_text
  )
  foreach(name_value ${config_text})
    if(name_value MATCHES "([^=]+)=([^\n]+)")
      set(name "${CMAKE_MATCH_1}")
      set(value "${CMAKE_MATCH_2}")
      if(NOT DEFINED "${name}")
        message("${name}=${value}")
        set("${name}" "${value}" CACHE INTERNAL "")
      endif()
    endif()
  endforeach()
endfunction()

include("${CMAKE_CURRENT_LIST_DIR}/config.cmake" OPTIONAL RESULT_VARIABLE OPENDDS_CONFIG_CMAKE)
if(NOT OPENDDS_CONFIG_CMAKE AND NOT ACE_IS_BEING_BUILT)
  if(DEFINED OPENDDS_ACE)
    if(NOT EXISTS "${OPENDDS_ACE}")
      message(FATAL_ERROR "OPENDDS_ACE does not exist")
    endif()
  else()
    message(FATAL_ERROR "OPENDDS_ACE must be defined")
  endif()
  get_filename_component(OPENDDS_ACE "${OPENDDS_ACE}" ABSOLUTE)
  if(DEFINED OPENDDS_TAO)
    if(NOT EXISTS "${OPENDDS_TAO}")
      message(FATAL_ERROR "OPENDDS_TAO does not exist")
    endif()
  else()
    if(EXISTS "${OPENDDS_ACE}/TAO")
      set(OPENDDS_TAO "${OPENDDS_ACE}/TAO")
    elseif(EXISTS "${OPENDDS_ACE}/../TAO")
      set(OPENDDS_TAO "${OPENDDS_ACE}/../TAO")
    else()
      message(FATAL_ERROR
        "OPENDDS_TAO not relative to OPENDDS_ACE, so OPENDDS_TAO must be defined")
    endif()
  endif()
  get_filename_component(OPENDDS_TAO "${OPENDDS_TAO}" ABSOLUTE)

  # TODO
  if(NOT WIN32)
  _opendds_detect_ace()
  endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/opendds_features.cmake")

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
  set(path_list)
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

  if(NOT OPENDDS_IS_BEING_BUILT)
    set(OPENDDS_BIN_DIR "${OPENDDS_ROOT}/bin")
    set(OPENDDS_LIB_DIR "${OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")
  endif()
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
