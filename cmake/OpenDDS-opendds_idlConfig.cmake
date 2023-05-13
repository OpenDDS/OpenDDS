# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

cmake_minimum_required(VERSION 3.3.2)

if(OpenDDS-opendds_idl_FOUND)
  return()
endif()

find_package(OpenDDS-TAO REQUIRED)

if(BUILDING_OPENDDS AND TARGET opendds_idl)
  set(OpenDDS-opendds_idl_FOUND TRUE)
else()
  set(OpenDDS-opendds_idl_FOUND FALSE)
  find_program(OPENDDS_IDL
    NAMES
      opendds_idl
    REQUIRED
    HINTS
      "${OPENDDS_BIN_DIR}"
  )
  _opendds_found_required_deps(OpenDDS-opendds_idl_FOUND OPENDDS_IDL)
  if(OpenDDS-opendds_idl_FOUND)
    _opendds_add_target_binary(opendds_idl "${OPENDDS_IDL}")
  endif()
endif()

if(OpenDDS-opendds_idl_FOUND)
  include("${CMAKE_CURRENT_LIST_DIR}/opendds_target_sources.cmake")
endif()
