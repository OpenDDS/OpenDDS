# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# CMake options for configuring the FindOpenDDS cmake module. These options
# depend upon variables/options from the config.cmake file which is auto-
# generated using the $DDS_ROOT/configure script.
#
# Other options which don't depend upon configuration-time settings are
# set. In addition, some base #defines are set depending on which options
# have been enabled. These are set on every target which generates IDL
# and/or links against respective MPC-Compiled targets (for example,
# OpenDDS::Dcps).
#

# Handle base property defines like MPC/config/dcps_optional_features.mpb
# depending on which cmd-line switches the user provided.

macro(_OPENDDS_APPEND_DEF)
  foreach(_def ${ARGN})
    list(APPEND OPENDDS_DCPS_COMPILE_DEFS ${_def})
    foreach(_defines_list
              OPENDDS_DDS_BASE_IDL_FLAGS
              OPENDDS_TAO_BASE_IDL_FLAGS)
      list(APPEND ${_defines_list} -D${_def})
    endforeach()
  endforeach()
endmacro()

if (NOT OPENDDS_BUILT_IN_TOPICS)
  _OPENDDS_APPEND_DEF(DDS_HAS_MINIMUM_BIT)
endif()

if (NOT OPENDDS_CONTENT_SUBSCRIPTION)
  _OPENDDS_APPEND_DEF(
    OPENDDS_NO_QUERY_CONDITION
    OPENDDS_NO_CONTENT_FILTERED_TOPIC
    OPENDDS_NO_MULTI_TOPIC
  )
endif()

if (NOT OPENDDS_QUERY_CONDITION)
  _OPENDDS_APPEND_DEF(OPENDDS_NO_QUERY_CONDITION)
endif()

if (NOT OPENDDS_CONTENT_FILTERED_TOPIC)
  _OPENDDS_APPEND_DEF(OPENDDS_NO_CONTENT_FILTERED_TOPIC)
endif()

if (NOT OPENDDS_MULTI_TOPIC)
  _OPENDDS_APPEND_DEF(OPENDDS_NO_MULTI_TOPIC)
endif()

if (NOT OPENDDS_OWNERSHIP_PROFILE)
  _OPENDDS_APPEND_DEF(
    OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    OPENDDS_NO_OWNERSHIP_PROFILE
  )
endif()

if (NOT OPENDDS_OWNERSHIP_KIND_EXCLUSIVE)
  _OPENDDS_APPEND_DEF(OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE)
endif()

if (NOT OPENDDS_OBJECT_MODEL_PROFILE)
  _OPENDDS_APPEND_DEF(OPENDDS_NO_OBJECT_MODEL_PROFILE)
endif()

if (NOT OPENDDS_PERSISTENCE_PROFILE)
  _OPENDDS_APPEND_DEF(OPENDDS_NO_PERSISTENCE_PROFILE)
endif()

if (OPENDDS_SECURITY)
  _OPENDDS_APPEND_DEF(OPENDDS_SECURITY)
endif()

# ACE defines.

if (OPENDDS_NO_DEBUG AND CMAKE_HOST_UNIX)
  _OPENDDS_APPEND_DEF(ACE_NDEBUG NDEBUG)
endif()

if (OPENDDS_NO_INLINE)
  _OPENDDS_APPEND_DEF(ACE_NO_INLINE)
endif()

if (OPENDDS_IPV6)
  _OPENDDS_APPEND_DEF(ACE_HAS_IPV6)
endif()

if (OPENDDS_STATIC)
  _OPENDDS_APPEND_DEF(ACE_AS_STATIC_LIBS ACE_HAS_CUSTOM_EXPORT_MACROS=0)
endif()

# Link libraries.

if (OPENDDS_XERCES3)
  list(APPEND OPENDDS_DCPS_LINK_DEPS ACE::XML_Utils)
endif()

# Force C++ standard.

if (OPENDDS_STD AND CMAKE_HOST_UNIX)
  if("${OPENDDS_STD}" MATCHES "(03|98)$")
    set(CMAKE_CXX_STANDARD 98)
  elseif("${OPENDDS_STD}" MATCHES "(0x|11)$")
    set(CMAKE_CXX_STANDARD 11)
  elseif("${OPENDDS_STD}" MATCHES "(1y|14)$")
    set(CMAKE_CXX_STANDARD 14)
  elseif("${OPENDDS_STD}" MATCHES "(1z|17)$")
    set(CMAKE_CXX_STANDARD 17)
  else()
    message(WARNING "Ignoring unknown OPENDDS_STD value '${OPENDDS_STD}'")
  endif()
endif()
