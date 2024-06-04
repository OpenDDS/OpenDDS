cmake_minimum_required(VERSION 3.23...3.27)
project(opendds_rtps CXX)

find_package(OpenDDS REQUIRED NO_DEFAULTS OpenDDS::Dcps OpenDDS::opendds_idl)
include(opendds_build_helpers)

add_library(OpenDDS_Rtps
  RtpsDiscoveryConfig.cpp
  RtpsDiscovery.cpp
  Sedp.cpp
  Spdp.cpp
  GuidGenerator.cpp
  ParameterListConverter.cpp
  MessageUtils.cpp
  MessageParser.cpp
  ICE/EndpointManager.cpp
  ICE/Task.cpp
  ICE/Stun.cpp
  ICE/Ice.cpp
  ICE/Checklist.cpp
  ICE/AgentImpl.cpp
)
target_sources(OpenDDS_Rtps
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    AssociationRecord.h
    DiscoveredEntities.h
    GuidGenerator.h
    ICE/AgentImpl.h
    ICE/Checklist.h
    ICE/EndpointManager.h
    ICE/Ice.h
    ICE/Stun.h
    ICE/Task.h
    LocalEntities.h
    MessageParser.h
    MessageTypes.h
    MessageUtils.h
    ParameterListConverter.h
    RtpsDiscovery.h
    RtpsDiscoveryConfig.h
    Sedp.h
    Spdp.h
    rtps_export.h
)
_opendds_library(OpenDDS_Rtps)
target_link_libraries(OpenDDS_Rtps PUBLIC OpenDDS::Dcps)

set(common_idl_options
  INCLUDE_BASE "${OPENDDS_SOURCE_DIR}"
  USE_EXPORT "dds/DCPS/RTPS/rtps_export.h" OpenDDS_Rtps_Export
)
set(tao_idl_options -St -Scdr)
set(opendds_idl_options -St -SI -Sv -Sx)
opendds_target_sources(OpenDDS_Rtps
  PUBLIC
    RtpsCore.idl
    RtpsRpc.idl
    TypeLookup.idl
  TAO_IDL_OPTIONS ${tao_idl_options}
  OPENDDS_IDL_OPTIONS ${opendds_idl_options}
  ${common_idl_options}
)
opendds_target_sources(OpenDDS_Rtps
  PUBLIC
    RtpsSubmessageKind.idl
  OPENDDS_IDL_OPTIONS ${opendds_idl_options} -Lspcpp
  SKIP_TAO_IDL
  ${common_idl_options}
)
opendds_target_sources(OpenDDS_Rtps
  PUBLIC
    RtpsSecurity.idl
  TAO_IDL_OPTIONS ${tao_idl_options}
  SKIP_OPENDDS_IDL
  ${common_idl_options}
)
opendds_install_interface_files(OpenDDS_Rtps INCLUDE_BASE "${OPENDDS_SOURCE_DIR}")
