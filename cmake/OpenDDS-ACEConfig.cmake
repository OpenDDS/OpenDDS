# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# find_package for ACE. See OpenDDSConfig.cmake for OpenDDS.

cmake_minimum_required(VERSION 3.3.2)

if(OpenDDS-ACE_FOUND)
  return()
endif()
set(OpenDDS-ACE_FOUND FALSE)

include("${CMAKE_CURRENT_LIST_DIR}/import_common.cmake")
if(NOT DEFINED ACE_ROOT)
  return()
endif()

set(_opendds_ace_required_deps ACE_LIBRARY ACE_GPERF)

if(OPENDDS_XERCES3)
  find_package(XercesC PATHS "${OPENDDS_XERCES3}" NO_DEFAULT_PATH)
  if(NOT XercesC_FOUND)
    find_package(XercesC)
  endif()
  if(NOT XercesC_FOUND)
    message(FATAL_ERROR "Could not find XercesC")
  endif()
endif()

function(_opendds_vs_force_static)
  # See https://gitlab.kitware.com/cmake/community/wikis/FAQ#dynamic-replace
  foreach(flag_var
          CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      set(${flag_var} "${${flag_var}}" PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

if(MSVC AND OPENDDS_STATIC)
  _opendds_vs_force_static()
endif()

find_program(ACE_GPERF
  NAMES
    ace_gperf
  # Adding ${TAO_BIN_DIR} to the ace bin hints allows users of
  # VxWorks layer builds to set TAO_BIN_DIR to the location of
  # the partner host tools directory, but keep ACE_BIN_DIR the
  # value of $ACE_ROOT so that other ACE related scripts can
  # be located.
  HINTS
    "${ACE_BIN_DIR}" "${TAO_BIN_DIR}"
)

set(_opendds_ace_libs ACE ACE_XML_Utils)

enable_language(C)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(ACE_DEPS Threads::Threads)

function(_opendds_add_ace_system_library name)
  list(APPEND ACE_DEPS "${name}")
  set(ACE_DEPS ${ACE_DEPS} PARENT_SCOPE)
  string(TOUPPER "${name}" cap_name)
  set(cap_name "${cap_name}_LIBRARY")
  if((${ARGC} GREATER 1) AND ("${ARGV1}" STREQUAL "NO_CHECK"))
    set("${cap_name}" "${name}" PARENT_SCOPE)
  else()
    find_library("${cap_name}" "${name}")
  endif()
  list(APPEND _opendds_ace_required_deps "${cap_name}")
  set(_opendds_ace_required_deps "${_opendds_ace_required_deps}" PARENT_SCOPE)
endfunction()

if(UNIX)
  _opendds_add_ace_system_library(dl)
  if(NOT APPLE)
    _opendds_add_ace_system_library(rt)
  endif()

  if(NOT OPENDDS_DEBUG)
    list(APPEND ACE_COMPILE_DEFINITIONS ACE_NDEBUG NDEBUG)
  endif()
elseif(MSVC)
  # For some reason CMake can't find this in some cases, but we know it should
  # be there, so just link to it without a check.
  _opendds_add_ace_system_library(iphlpapi NO_CHECK)

  # ACE provides (or uses) many deprecated functions.
  # For now, we'll silence the warnings.
  list(APPEND ACE_COMPILE_OPTIONS "/wd4996")
  list(APPEND ACE_COMPILE_DEFINITIONS
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
  )
endif()

if(NOT MSVC) # On MSVC, ACE sets this in config-win32-common.h
  if(OPENDDS_INLINE)
    list(APPEND ACE_COMPILE_DEFINITIONS __ACE_INLINE__)
  else()
    list(APPEND ACE_COMPILE_DEFINITIONS ACE_NO_INLINE)
  endif()
endif()

if(OPENDDS_IPV6)
  list(APPEND ACE_COMPILE_DEFINITIONS ACE_HAS_IPV6)
endif()

if(OPENDDS_STATIC)
  list(APPEND ACE_COMPILE_DEFINITIONS
    ACE_AS_STATIC_LIBS
    ACE_HAS_CUSTOM_EXPORT_MACROS=0
  )
endif()

if(OPENDDS_WCHAR)
  list(APPEND ACE_COMPILE_DEFINITIONS ACE_USES_WCHAR)
endif()

if(OPENDDS_VERSIONED_NAMESPACE)
  list(APPEND ACE_COMPILE_DEFINITIONS ACE_HAS_VERSIONED_NAMESPACE=1)
endif()

set(ACE_XML_UTILS_DEPS ACE::ACE XercesC::XercesC)

_opendds_find_our_libraries("ACE" "${_opendds_ace_libs}")
_opendds_found_required_deps(OpenDDS-ACE_FOUND "${_opendds_ace_required_deps}")
if(OpenDDS-ACE_FOUND)
  _opendds_add_target_binary(ace_gperf "${ACE_GPERF}")
  _opendds_add_library_group("ACE" "${_opendds_ace_libs}" TRUE)
endif()
