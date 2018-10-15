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
    foreach(_defines_list
              OPENDDS_DCPS_COMPILE_DEFS
              OPENDDS_DDS_BASE_IDL_FLAGS
              OPENDDS_TAO_BASE_IDL_FLAGS)
      list(APPEND ${_defines_list} ${_def})
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

# Handle configure-generated link dependencies.

if (OPENDDS_XERCES3)
  list(APPEND OPENDDS_DCPS_LINK_DEPS ACE::XML_Utils)
endif()
