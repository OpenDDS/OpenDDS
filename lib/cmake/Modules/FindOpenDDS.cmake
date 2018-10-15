# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

#.rst:
# FindOpenDDS
# -----------
#
# Finds OpenDDS include dirs and libraries
#
# This module defines the following variables::
#
# OPENDDS_FOUND - True if OpenDDS was found.
# OPENDDS_INCLUDE_DIRS - Directories containing OpenDDS include files.
# OPENDDS_LIBRARIES - Libraries needed to link against OpenDDS.
# OPENDDS_DEFINITIONS - Compiler flags required by OpenDDS.
# OPENDDS_VERSION - Full OpenDDS version string.
# OPENDDS_VERSION_MAJOR - Major version of OpenDDS.
# OPENDDS_VERSION_MINOR - Minor version of OpenDDS.
# OPENDDS_VERSION_PATCH - Patch version of OpenDDS.

if (NOT DEFINED DDS_ROOT)
  set(DDS_ROOT $ENV{DDS_ROOT})
endif()

if (NOT DEFINED ACE_ROOT)
  set(ACE_ROOT $ENV{ACE_ROOT})
endif()

if (NOT DEFINED TAO_ROOT)
  set(TAO_ROOT $ENV{TAO_ROOT})
endif()

if (MSVC)
  set(CMAKE_MSVCIDE_RUN_PATH ${CMAKE_MSVCIDE_RUN_PATH} ${DDS_ROOT}/lib ${ACE_ROOT}/lib)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/FindOpenDDS/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/FindOpenDDS/options.cmake)

find_path(OPENDDS_INCLUDE_DIR dds HINTS ${DDS_ROOT})
find_path(OPENDDS_BIN_DIR bin HINTS ${DDS_ROOT})
find_path(OPENDDS_LIB_DIR lib HINTS ${DDS_ROOT})
find_path(ACE_INCLUDE_DIR ace HINTS ${ACE_ROOT})
find_path(ACE_BIN_DIR bin HINTS ${ACE_ROOT})
find_path(ACE_LIB_DIR lib HINTS ${ACE_ROOT})
find_path(TAO_INCLUDE_DIR tao HINTS ${TAO_ROOT})
set(TAO_BIN_DIR ${ACE_BIN_DIR})
set(TAO_LIB_DIR ${ACE_LIB_DIR})

set(_dds_bin_hints ${OPENDDS_BIN_DIR} $ENV{DDS_ROOT}/bin)
set(_tao_bin_hints ${ACE_BIN_DIR} $ENV{ACE_ROOT}/bin)
set(_ace_bin_hints ${TAO_BIN_DIR} $ENV{TAO_ROOT}/bin)

find_program(PERL perl)

find_program(OPENDDS_IDL
  NAMES
    opendds_idl
  HINTS
    ${_dds_bin_hints}
)

find_program(TAO_IDL
  NAMES
    tao_idl
  HINTS
    ${_tao_bin_hints}
)

find_program(ACE_GPERF
  NAMES
    ace_gperf
  HINTS
    ${_ace_bin_hints}
)

set(_ace_libs
  ACE_XML_Utils
  ACE
)

set(_tao_libs
  TAO_IORManip
  TAO_ImR_Client
  TAO_Svc_Utils
  TAO_IORTable
  TAO_IDL_FE
  TAO_PortableServer
  TAO_BiDirGIOP
  TAO_PI
  TAO_CodecFactory
  TAO_AnyTypeCode
  TAO
)

set(_opendds_libs
  OpenDDS_Dcps
  OpenDDS_FACE
  OpenDDS_Federator
  OpenDDS_InfoRepoDiscovery
  OpenDDS_InfoRepoLib
  OpenDDS_InfoRepoServ
  OpenDDS_Model
  OpenDDS_monitor
  OpenDDS_Multicast
  OpenDDS_QOS_XML_XSC_Handler
  OpenDDS_Rtps
  OpenDDS_Rtps_Udp
  OpenDDS_Security
  OpenDDS_Shmem
  OpenDDS_Tcp
  OpenDDS_Udp
)

list(APPEND _all_libs ${_opendds_libs} ${_ace_libs} ${_tao_libs})

set(OPENDDS_IDL_DEPS
  TAO::IDL_FE
  ACE::ACE
)

set(OPENDDS_DCPS_DEPS
  TAO::PortableServer
  TAO::BiDirGIOP
  TAO::PI
  TAO::CodecFactory
  TAO::AnyTypeCode
  TAO::TAO
  ACE::ACE
)

set(OPENDDS_FACE_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_FEDERATOR_DEPS
  OpenDDS::InfoRepoLib
)

set(OPENDDS_INFOREPODISCOVERY_DEPS
  OpenDDS::Tcp
  OpenDDS::Dcps
)

set(OPENDDS_INFOREPOLIB_DEPS
  OpenDDS::InfoRepoDiscovery
  TAO::Svc_Utils
  TAO::ImR_Client
  TAO::IORManip
  TAO::IORTable
)

set(OPENDDS_INFOREPOSERV_DEPS
  OpenDDS::Federator
)

set(OPENDDS_MODEL_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_MONITOR_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_MULTICAST_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_QOS_XML_XSC_HANDLER_DEPS
  OpenDDS::Dcps
  ACE::XML_Utils
)

set(OPENDDS_RTPS_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_RTPS_UDP_DEPS
  OpenDDS::Rtps
)

set(OPENDDS_SECURITY_DEPS
  OpenDDS::Rtps
  ACE::XML_Utils
)

set(OPENDDS_SHMEM_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_TCP_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_UDP_DEPS
  OpenDDS::Dcps
)

set(_dds_lib_hints  ${OPENDDS_LIB_DIR}  $ENV{DDS_ROOT}/lib)
set(_ace_lib_hints  ${ACE_LIB_DIR}  $ENV{ACE_ROOT}/lib)
set(_tao_lib_hints  ${TAO_LIB_DIR}  $ENV{TAO_ROOT}/lib)

set(_suffix_RELEASE "")
set(_suffix_DEBUG d)
foreach(_cfg  RELEASE  DEBUG)
  set(_sfx ${_suffix_${_cfg}})

  foreach(_lib ${_ace_libs})
    string(TOUPPER ${_lib} _LIB_VAR)

    find_library(${_LIB_VAR}_LIBRARY_${_cfg}
      ${_lib}${_sfx}
      HINTS ${_ace_lib_hints}
    )
  endforeach()

  foreach(_lib ${_tao_libs})
    string(TOUPPER ${_lib} _LIB_VAR)

    find_library(${_LIB_VAR}_LIBRARY_${_cfg}
      ${_lib}${_sfx}
      # By default TAO libraries are built into ACE_ROOT/lib
      # so the hints are shared here.
      HINTS ${_tao_lib_hints} ${_ace_lib_hints}
    )
  endforeach()

  foreach(_lib ${_opendds_libs})
    string(TOUPPER ${_lib} _LIB_VAR)

    find_library(${_LIB_VAR}_LIBRARY_${_cfg}
      ${_lib}${_sfx}
      HINTS ${_dds_lib_hints}
    )
  endforeach()

endforeach()

function(opendds_extract_version  in_version_file  out_version  out_major  out_minor)
  file(READ "${in_version_file}" contents)
  if(contents)
    string(REGEX MATCH "OpenDDS version (([0-9]+).([0-9]+))" _ "${contents}")
    set(${out_version} ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(${out_major}   ${CMAKE_MATCH_2} PARENT_SCOPE)
    set(${out_minor}   ${CMAKE_MATCH_3} PARENT_SCOPE)
  endif()
endfunction()

opendds_extract_version("${DDS_ROOT}/VERSION"
  OPENDDS_VERSION
  OPENDDS_VERSION_MAJOR
  OPENDDS_VERSION_MINOR
)

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

foreach(_lib ${_all_libs})
  string(TOUPPER ${_lib} _LIB_VAR)
  select_library_configurations(${_LIB_VAR})

  if(${${_LIB_VAR}_FOUND})
    message(STATUS "${_lib} -> ${${_LIB_VAR}_LIBRARY}")
  endif()
endforeach()

find_package_handle_standard_args(OPENDDS
  FOUND_VAR OPENDDS_FOUND
  REQUIRED_VARS
    OPENDDS_INCLUDE_DIR
    OPENDDS_DCPS_LIBRARY
    OPENDDS_IDL
    ACE_LIBRARY
    ACE_GPERF
    TAO_LIBRARY
    TAO_IDL
    PERL
  VERSION_VAR OPENDDS_VERSION
)

macro(_ADD_TARGET_BINARY  target  path)
  if (NOT TARGET ${target} AND EXISTS "${path}")
    add_executable(${target} IMPORTED)
    set_target_properties(${target}
      PROPERTIES
        IMPORTED_LOCATION "${path}"
    )
  endif()
endmacro()

macro(_ADD_TARGET_LIB  target  var_prefix  include_dir)
  set(_debug_lib "${${var_prefix}_LIBRARY_DEBUG}")
  set(_release_lib "${${var_prefix}_LIBRARY_RELEASE}")
  set(_deps "${${var_prefix}_DEPS}")

  if (NOT TARGET ${target} AND
      (EXISTS "${_debug_lib}" OR EXISTS "${_release_lib}"))

    add_library(${target} UNKNOWN IMPORTED)
    set_target_properties(${target}
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
        INTERFACE_LINK_LIBRARIES "${_deps}"
    )

    if (EXISTS "${_release_lib}")
      set_property(TARGET ${target}
        APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE
      )
      set_target_properties(${target}
        PROPERTIES
          IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
          IMPORTED_LOCATION_RELEASE "${_release_lib}"
      )
    endif()

    if (EXISTS "${_debug_lib}")
      set_property(TARGET ${target}
        APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG
      )
      set_target_properties(${target}
        PROPERTIES
          IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
          IMPORTED_LOCATION_DEBUG "${_debug_lib}"
      )
    endif()

    list(APPEND OPENDDS_LIBRARIES ${${var_prefix}})

  endif()
endmacro()

if(OPENDDS_FOUND)
  set(OPENDDS_INCLUDE_DIRS
      ${OPENDDS_INCLUDE_DIR}
      ${ACE_INCLUDE_DIR}
      ${TAO_INCLUDE_DIR}
      ${TAO_INCLUDE_DIR}/orbsvcs
  )

  _ADD_TARGET_BINARY(opendds_idl "${OPENDDS_IDL}")
  _ADD_TARGET_BINARY(tao_idl "${TAO_IDL}")
  _ADD_TARGET_BINARY(ace_gperf "${ACE_GPERF}")
  _ADD_TARGET_BINARY(perl "${PERL}")

  foreach(_lib ${_ace_libs})
    string(TOUPPER ${_lib} _VAR_PREFIX)

    if("${_lib}" STREQUAL "ACE")
      set(_target "ACE::ACE")
    else()
      string(REPLACE "ACE_" "ACE::" _target ${_lib})
    endif()

    _ADD_TARGET_LIB(${_target} ${_VAR_PREFIX} "${ACE_INCLUDE_DIR}")
  endforeach()

  foreach(_lib ${_tao_libs})
    string(TOUPPER ${_lib} _VAR_PREFIX)

    if("${_lib}" STREQUAL "TAO")
      set(_target "TAO::TAO")
    else()
      string(REPLACE "TAO_" "TAO::" _target ${_lib})
    endif()

    _ADD_TARGET_LIB(${_target} ${_VAR_PREFIX} "${TAO_INCLUDE_DIR}")
  endforeach()

  foreach(_lib ${_opendds_libs})
    string(TOUPPER ${_lib} _VAR_PREFIX)
    string(REPLACE "OpenDDS_" "OpenDDS::" _target ${_lib})

    _ADD_TARGET_LIB(${_target} ${_VAR_PREFIX} "${OPENDDS_INCLUDE_DIR}")

  endforeach()

  if(NOT TARGET OpenDDS::OpenDDS)
    add_library(OpenDDS::OpenDDS INTERFACE IMPORTED)

    set(_opendds_core_libs
      OpenDDS::Dcps
      OpenDDS::Multicast
      OpenDDS::Rtps
      OpenDDS::Rtps_Udp
      OpenDDS::InfoRepoDiscovery
      OpenDDS::Shmem
      OpenDDS::Tcp
      OpenDDS::Udp)

    if(OPENDDS_SECURITY)
      list(APPEND _opendds_core_libs OpenDDS::Security)
    endif()

    target_link_libraries(OpenDDS::OpenDDS INTERFACE ${_opendds_core_libs})
  endif()

  include(${CMAKE_CURRENT_LIST_DIR}/FindOpenDDS/api_macros.cmake)
endif()
