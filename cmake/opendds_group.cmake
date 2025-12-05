# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# Definitions for the OpenDDS libraries that are shared between the CMake
# config package for an MPC-built OpenDDS and a CMake-built OpenDDS.

if(_OPENDDS_GROUP_CMAKE)
  return()
endif()
set(_OPENDDS_GROUP_CMAKE TRUE)

include("${CMAKE_CURRENT_LIST_DIR}/init.cmake")

_opendds_group(OpenDDS DEFAULT_REQUIRED OpenDDS::Dcps OpenDDS::opendds_idl)

_opendds_group_lib(Dcps
  DEPENDS
    ACE::ACE
    # TODO: These are omitted with safety profile
    TAO::TAO
    TAO::Valuetype
    TAO::PortableServer
    TAO::BiDirGIOP
)
_opendds_group_lib(FACE DEPENDS OpenDDS::Dcps)
_opendds_group_lib(Federator DEPENDS OpenDDS::InfoRepoLib)
_opendds_group_lib(InfoRepoDiscovery
  DEPENDS
    OpenDDS::Dcps
    OpenDDS::Tcp
    TAO::PortableServer
    TAO::BiDirGIOP
    TAO::PI
    TAO::CodecFactory
    TAO::Valuetype
    TAO::AnyTypeCode
)
_opendds_group_lib(InfoRepoLib
  DEPENDS
    OpenDDS::InfoRepoDiscovery
    TAO::Svc_Utils
    TAO::ImR_Client
    TAO::IORManip
    TAO::IORTable
)
_opendds_group_lib(InfoRepoServ DEPENDS OpenDDS::Federator)
_opendds_group_lib(Model DEPENDS OpenDDS::Dcps)
_opendds_group_lib(monitor DEPENDS OpenDDS::Dcps)
_opendds_group_lib(Multicast DEPENDS OpenDDS::Dcps)
_opendds_group_lib(Rtps DEPENDS OpenDDS::Dcps)
_opendds_group_lib(Rtps_Udp DEPENDS OpenDDS::Rtps)
_opendds_group_lib(Security
  DEPENDS
    OpenDDS::Rtps
    ACE::XML_Utils
    OpenSSL::SSL
    OpenSSL::Crypto
)
_opendds_group_lib(Shmem DEPENDS OpenDDS::Dcps)
_opendds_group_lib(Tcp DEPENDS OpenDDS::Dcps)
_opendds_group_lib(Udp DEPENDS OpenDDS::Dcps)
_opendds_group_lib(QOS_XML_XSC_Handler DEPENDS OpenDDS::Dcps ACE::XML_Utils)
_opendds_group_lib(RtpsRelayLib DEPENDS OpenDDS::Dcps)

_opendds_group_exe(opendds_idl HOST_TOOL)
foreach(_exe DCPSInfoRepo RtpsRelay RtpsRelayControl dcpsinfo_dump inspect repoctl)
  _opendds_group_exe("${_exe}")
endforeach()

if(OPENDDS_SECURITY)
  find_package(OpenSSL PATHS "${OPENDDS_OPENSSL}" NO_DEFAULT_PATH QUIET)
  if(NOT OpenSSL_FOUND)
    set(OPENSSL_ROOT_DIR "${OPENDDS_OPENSSL}")
    find_package(OpenSSL QUIET)
  endif()
  if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "Could not find OpenSSL")
  endif()
endif()

set(OPENDDS_DCPS_INCLUDE_DIRS ${OPENDDS_INCLUDE_DIRS})
set(OPENDDS_DCPS_COMPILE_DEFINITIONS)
if(OPENDDS_RAPIDJSON)
  list(APPEND OPENDDS_DCPS_INCLUDE_DIRS "${OPENDDS_RAPIDJSON}/include")
  list(APPEND OPENDDS_DCPS_COMPILE_DEFINITIONS OPENDDS_RAPIDJSON)
endif()

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
