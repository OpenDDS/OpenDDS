# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

cmake_minimum_required(VERSION 3.3.2)

if(OpenDDS_FOUND)
  return()
endif()
set(OpenDDS_FOUND FALSE)

find_package(OpenDDS-TAO REQUIRED)
find_package(OpenDDS-opendds_idl REQUIRED)

if(OPENDDS_SECURITY)
  find_package(OpenSSL PATHS "${OPENDDS_OPENSSL}" NO_DEFAULT_PATH)
  if (NOT OpenSSL_FOUND)
    set(OPENSSL_ROOT_DIR "${OPENDDS_OPENSSL}")
    find_package(OpenSSL)
  endif()
  if (NOT OpenSSL_FOUND)
    message(FATAL_ERROR "Could not find OpenSSL")
  endif()

  if(NOT OPENDDS_XERCES3)
    set(OPENDDS_XERCES3 ON)
  endif()
endif()

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
  OpenDDS_Rtps
  OpenDDS_Rtps_Udp
  OpenDDS_Shmem
  OpenDDS_Tcp
  OpenDDS_Udp
  OpenDDS_QOS_XML_XSC_Handler
  OpenDDS_Security
)

set(OPENDDS_DCPS_DEPS
  ACE::ACE
  TAO::TAO
  # TODO: These are omitted with safety profile
  TAO::Valuetype
  TAO::PortableServer
)

if(NOT OPENDDS_BUILT_IN_TOPICS)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS DDS_HAS_MINIMUM_BIT)
endif()

if(NOT OPENDDS_CONTENT_SUBSCRIPTION)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS
    OPENDDS_NO_QUERY_CONDITION
    OPENDDS_NO_CONTENT_FILTERED_TOPIC
    OPENDDS_NO_MULTI_TOPIC
  )
endif()

if(NOT OPENDDS_QUERY_CONDITION)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_NO_QUERY_CONDITION)
endif()

if(NOT OPENDDS_CONTENT_FILTERED_TOPIC)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_NO_CONTENT_FILTERED_TOPIC)
endif()

if(NOT OPENDDS_MULTI_TOPIC)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_NO_MULTI_TOPIC)
endif()

if(NOT OPENDDS_OWNERSHIP_PROFILE)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS
    OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    OPENDDS_NO_OWNERSHIP_PROFILE
  )
endif()

if(NOT OPENDDS_OWNERSHIP_KIND_EXCLUSIVE)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE)
endif()

if(NOT OPENDDS_OBJECT_MODEL_PROFILE)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_NO_OBJECT_MODEL_PROFILE)
endif()

if(NOT OPENDDS_PERSISTENCE_PROFILE)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_NO_PERSISTENCE_PROFILE)
endif()

if(OPENDDS_SECURITY)
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_SECURITY)
endif()

set(OPENDDS_FACE_DEPS
  OpenDDS::Dcps
)

set(OPENDDS_FEDERATOR_DEPS
  OpenDDS::InfoRepoLib
)

set(OPENDDS_INFOREPODISCOVERY_DEPS
  OpenDDS::Dcps
  TAO::PortableServer
  TAO::BiDirGIOP
  TAO::PI
  TAO::CodecFactory
  TAO::Valuetype
  TAO::AnyTypeCode
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
  OpenSSL::SSL
  OpenSSL::Crypto
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

_opendds_find_our_libraries("OPENDDS" "${_opendds_libs}")
_opendds_found_required_deps(OpenDDS_FOUND OPENDDS_DCPS_LIBRARY)

if(OpenDDS_FOUND)
  _opendds_add_library_group("OpenDDS" "${_opendds_libs}" FALSE)

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
      OpenDDS::Udp
    )
    if(OPENDDS_SECURITY)
      list(APPEND _opendds_core_libs OpenDDS::Security)
    endif()

    set_target_properties(OpenDDS::OpenDDS
      PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_opendds_core_libs}")
  endif()
endif()
