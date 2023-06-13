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
_opendds_library(OpenDDS_Rtps)
target_link_libraries(OpenDDS_Rtps PUBLIC OpenDDS::Dcps)

set(ots_args
  INCLUDE_BASE "${OPENDDS_SOURCE_ROOT}"
  USE_EXPORT "dds/DCPS/RTPS/rtps_export.h" OpenDDS_Rtps_Export
)
opendds_target_sources(OpenDDS_Rtps
  PUBLIC
    RtpsCore.idl
    RtpsSecurity.idl
    RtpsRpc.idl
    TypeLookup.idl
  ${ots_args}
  SKIP_OPENDDS_IDL
  TAO_IDL_OPTIONS -St -Scdr
)
opendds_target_sources(OpenDDS_Rtps
  PUBLIC
    RtpsCore.idl
    RtpsRpc.idl
    TypeLookup.idl
  ${ots_args}
  SKIP_TAO_IDL
)
