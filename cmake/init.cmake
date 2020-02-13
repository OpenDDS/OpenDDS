# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/config.cmake)

# Make Sure CMake can use the Paths
file(TO_CMAKE_PATH "${OPENDDS_ACE}" OPENDDS_ACE)
file(TO_CMAKE_PATH "${OPENDDS_MPC}" OPENDDS_MPC)
file(TO_CMAKE_PATH "${OPENDDS_TAO}" OPENDDS_TAO)

option(OPENDDS_CMAKE_VERBOSE "Print verbose output when loading the OpenDDS Config Package" OFF)
option(OPENDDS_DEFAULT_NESTED "Require topic types to be declared explicitly" ON)

set(_OPENDDS_RELATIVE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(_OPENDDS_RELATIVE_PREFIX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

get_filename_component(_OPENDDS_RELATIVE_SOURCE_ROOT
  ${_OPENDDS_RELATIVE_SOURCE_ROOT} ABSOLUTE)

get_filename_component(_OPENDDS_RELATIVE_PREFIX_ROOT
  ${_OPENDDS_RELATIVE_PREFIX_ROOT} ABSOLUTE)

macro(_OPENDDS_RETURN_ERR msg)
  message(SEND_ERROR "${msg}")
  set(OPENDDS_FOUND FALSE)
  return()
endmacro()

if(NOT DEFINED DDS_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(DDS_ROOT "${_OPENDDS_RELATIVE_PREFIX_ROOT}/share/dds")
    set(OPENDDS_INCLUDE_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/include")
    set(OPENDDS_BIN_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/bin")
    set(OPENDDS_LIB_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/lib")

  else()
    set(DDS_ROOT ${_OPENDDS_RELATIVE_SOURCE_ROOT})
    set(OPENDDS_INCLUDE_DIR "${DDS_ROOT}")
    set(OPENDDS_BIN_DIR "${DDS_ROOT}/bin")
    set(OPENDDS_LIB_DIR "${DDS_ROOT}/lib")
  endif()

else()
  _OPENDDS_RETURN_ERR("DDS_ROOT has already been set")
endif()

if (NOT DEFINED ACE_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(ACE_ROOT "${_OPENDDS_RELATIVE_PREFIX_ROOT}/share/ace")
    set(ACE_INCLUDE_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/include")
    set(ACE_LIB_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/lib")

  elseif(OPENDDS_ACE)
    set(ACE_ROOT ${OPENDDS_ACE})
    set(ACE_INCLUDE_DIR "${ACE_ROOT}")
    set(ACE_LIB_DIR "${ACE_ROOT}/lib")

  else()
    _OPENDDS_RETURN_ERR("Failed to locate ACE_ROOT")
  endif()

  set(ACE_BIN_DIR "${ACE_ROOT}/bin")

else()
  _OPENDDS_RETURN_ERR("ACE_ROOT has already been set")
endif()

if (NOT DEFINED TAO_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(TAO_ROOT "${_OPENDDS_RELATIVE_PREFIX_ROOT}/share/tao")
    set(TAO_INCLUDE_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/include")
    set(TAO_BIN_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/bin")
    set(TAO_LIB_DIR "${_OPENDDS_RELATIVE_PREFIX_ROOT}/lib")

  elseif(OPENDDS_TAO)
    set(TAO_ROOT "${OPENDDS_TAO}")
    set(TAO_INCLUDE_DIR "${OPENDDS_TAO}")
    set(TAO_BIN_DIR "${ACE_BIN_DIR}")
    set(TAO_LIB_DIR "${ACE_LIB_DIR}")

  else()
    _OPENDDS_RETURN_ERR("Failed to locate TAO_ROOT")
  endif()

else()
  _OPENDDS_RETURN_ERR("TAO_ROOT has already been set")
endif()
