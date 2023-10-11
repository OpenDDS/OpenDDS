# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

cmake_minimum_required(VERSION 3.3.2...3.27)

if(_OPENDDS_ACE_GROUP_CMAKE)
  return()
endif()
set(_OPENDDS_ACE_GROUP_CMAKE TRUE)

include("${CMAKE_CURRENT_LIST_DIR}/import_common.cmake")

_opendds_group(ACE DEFAULT_REQUIRED ACE::ACE)

if(_OPENDDS_ACE_MPC_NAME_IS_ACE_TARGET)
  set(_mpc_name ACE-target)
else()
  set(_mpc_name ACE)
endif()
_opendds_group_lib(ACE
  MPC_NAME "${_mpc_name}"
  DEPENDS Threads::Threads
)
_opendds_group_lib(XML_Utils
  MPC_NAME ACE_XML_Utils
  DEPENDS ACE::ACE XercesC::XercesC
)

_opendds_group_exe(ace_gperf
  MPC_NAME gperf
  DEPENDS ACE::ACE
  HOST_TOOL
  # Adding ${TAO_BIN_DIR} to the ace bin hints allows users of
  # VxWorks layer builds to set TAO_BIN_DIR to the location of
  # the partner host tools directory, but keep ACE_BIN_DIR the
  # value of $ACE_ROOT so that other ACE related scripts can
  # be located.
  EXTRA_BIN_DIRS "${TAO_BIN_DIR}"
)

if(OPENDDS_XERCES3)
  find_package(XercesC PATHS "${OPENDDS_XERCES3}" NO_DEFAULT_PATH QUIET)
  if(NOT XercesC_FOUND)
    find_package(XercesC QUIET)
  endif()
  if(NOT XercesC_FOUND)
    message(FATAL_ERROR "Could not find XercesC")
  endif()
endif()

function(_opendds_vs_force_static)
  # Make sure the MSVC runtime library, which is similar to libc of other
  # systems, is the same kind everywhere. Normally we shouldn't make global
  # changes, but if we don't do this, MSVC won't link the programs if the
  # runtimes of compiled objects are different.
  # This is based on an old CMake FAQ:
  # https://web.archive.org/web/20210319004154/https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#dynamic-replace
  # At some point at or before CMake 3.26 these became cache variables.
  # There are alternative methods of making this work, like
  # MSVC_RUNTIME_LIBRARY, but this method seems like it works best for our
  # purposes.
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" new_value "${${flag_var}}")
      set(${flag_var} "${new_value}" CACHE INTERNAL
        "CXX Flags (Overridden by OpenDDS to match /MT* with ACE)")
    endif()
  endforeach()
endfunction()

if(MSVC AND OPENDDS_STATIC)
  _opendds_vs_force_static()
endif()

enable_language(C)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

function(_opendds_add_ace_system_library name)
  _opendds_append(ACE_ACE_DEPENDS "${name}")
  string(TOUPPER "${name}" cap_name)
  set(cap_name "${cap_name}_LIBRARY")
  if((${ARGC} GREATER 1) AND ("${ARGV1}" STREQUAL "NO_CHECK"))
    set("${cap_name}" "${name}" PARENT_SCOPE)
  else()
    find_library("${cap_name}" "${name}")
  endif()
  _opendds_append(ACE_ACE_DEFAULT_REQUIRED "${cap_name}")
endfunction()

if(UNIX)
  _opendds_add_ace_system_library(dl)
  if(NOT APPLE AND NOT ANDROID)
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
