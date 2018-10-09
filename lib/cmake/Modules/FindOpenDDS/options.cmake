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

set(OPENDDS_SAFETY_PROFILE NO CACHE STRING "")
set_property(CACHE OPENDDS_SAFETY_PROFILE PROPERTY STRINGS NO BASE EXTENDED)
if (OPENDDS_SAFETY_PROFILE)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFS OPENDDS_SAFETY_PROFILE)
endif()

set(OPENDDS_BASE_OPTIONS
      OPENDDS_QUERY_CONDITION
      OPENDDS_CONTENT_FILTERED_TOPIC
      OPENDDS_MULTI_TOPIC
      OPENDDS_OWNERSHIP_KIND_EXCLUSIVE
      OPENDDS_OBJECT_MODEL_PROFILE
      OPENDDS_PERSISTENCE_PROFILE
)

foreach(opt ${OPENDDS_BASE_OPTIONS})
  if (OPENDDS_SAFETY_PROFILE)
    option(${opt} "" OFF)
  endif()
endforeach()

list(APPEND OPENDDS_BASE_OPTIONS
     OPENDDS_CONTENT_SUBSCRIPTION
     OPENDDS_OWNERSHIP_PROFILE)

if (NOT OPENDDS_CONTENT_SUBSCRIPTION)
  set(OPENDDS_QUERY_CONDITION OFF)
  set(OPENDDS_CONTENT_FILTERED_TOPIC OFF)
  set(OPENDDS_MULTI_TOPIC OFF)
endif()

if (NOT OPENDDS_OWNERSHIP_PROFILE)
  # Currently there is no support for exclusion of code dealing with HISTORY depth > 1
  # therefore ownership_profile is the same as ownership_kind_exclusive.
  set(OPENDDS_OWNERSHIP_KIND_EXCLUSIVE OFF)
endif()

option(OPENDDS_SUPPRESS_ANYS "" ON)
option(OPENDDS_USE_UNIQUE_PTR_EMULATION "Do not use std::unqiue_ptr even when C++11 or above is detected." OFF)


if (OPENDDS_CONTENT_SUBSCRIPTION AND (OPENDDS_QUERY_CONDITION OR OPENDDS_CONTENT_FILTERED_TOPIC OR OPENDDS_MULTI_TOPIC))
  set(OPENDDS_CONTENT_SUBSCRIPTION_CORE TRUE)
endif()

foreach(opt ${OPENDDS_BASE_OPTIONS})
  if (NOT ${opt})
    string(REPLACE OPENDDS OPENDDS_NO inverted_opt ${opt})
    list(APPEND OPENDDS_DCPS_COMPILE_DEFS ${inverted_opt})
  endif()
endforeach()

if (OPENDDS_USE_UNIQUE_PTR_EMULATION)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFS OPENDDS_USE_UNIQUE_PTR_EMULATION)
endif(OPENDDS_USE_UNIQUE_PTR_EMULATION)

if (NOT OPENDDS_BUILT_IN_TOPICS)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFS DDS_MINIMUM_BIT)
endif()

if (OPENDDS_SECURITY AND OPENDDS_SAFETY_PROFILE)
  option(OPENDDS_SECURITY "" OFF)
endif()

if (OPENDDS_SECURITY)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFS OPENDDS_SECURITY)
endif()

if (NOT DEFINED OPENDDS_DDS_BASE_IDL_FLAGS)
  if (OPENDDS_SUPPRESS_ANYS)
    list(APPEND OPENDDS_TAO_BASE_IDL_FLAGS -Sa -St)
    list(APPEND OPENDDS_DDS_BASE_IDL_FLAGS -Sa -St)
  endif()
  foreach(definition ${OPENDDS_DCPS_COMPILE_DEFS})
    list(APPEND OPENDDS_TAO_BASE_IDL_FLAGS -D${definition})
    list(APPEND OPENDDS_DDS_BASE_IDL_FLAGS -D${definition})
  endforeach()
endif()
