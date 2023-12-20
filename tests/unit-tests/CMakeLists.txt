cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_unit_tests CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(${PROJECT_NAME}
  UnitTests.cpp
  tools/dds/rtpsrelaylib/Name.cpp
  tools/dds/rtpsrelaylib/Utility.cpp
  tools/dds/rtpsrelaylib/PartitionIndex.cpp
  dds/DCPS/WaitSet.cpp
  dds/DCPS/NetworkAddress.cpp
  dds/DCPS/GuidConverter.cpp
  dds/DCPS/ConfigStoreImpl.cpp
  dds/DCPS/Message_Block_Ptr.cpp
  dds/DCPS/JsonValueReader.cpp
  dds/DCPS/FibonacciSequence.cpp
  dds/DCPS/DispatchService.cpp
  dds/DCPS/DisjointSequence.cpp
  dds/DCPS/Qos_Helper.cpp
  dds/DCPS/AtomicBool.cpp
  dds/DCPS/ThreadPool.cpp
  dds/DCPS/SafetyProfileStreams.cpp
  dds/DCPS/PrinterValueWriter.cpp
  dds/DCPS/RcObject.cpp
  dds/DCPS/InternalDataReader.cpp
  dds/DCPS/SporadicTask.cpp
  dds/DCPS/Serializer.cpp
  dds/DCPS/transport/rtps_udp/MetaSubmessage.cpp
  dds/DCPS/transport/rtps_udp/RtpsSampleHeader.cpp
  dds/DCPS/transport/rtps_udp/TransactionalRtpsSendQueue.cpp
  dds/DCPS/transport/framework/TransportReassembly.cpp
  dds/DCPS/transport/framework/CopyChainVisitor.cpp
  dds/DCPS/transport/framework/ReceivedDataSample.cpp
  dds/DCPS/transport/framework/TransportStatistics.cpp
  dds/DCPS/transport/framework/TransportInst.cpp
  dds/DCPS/transport/framework/MessageDropper.cpp
  dds/DCPS/InternalDataWriter.cpp
  dds/DCPS/MemoryPool.cpp
  dds/DCPS/GuidUtils.cpp
  dds/DCPS/PeriodicEvent.cpp
  dds/DCPS/SporadicEvent.cpp
  dds/DCPS/RTPS/LocalEntities.cpp
  dds/DCPS/RTPS/AssociationRecord.cpp
  dds/DCPS/RTPS/GuidGenerator.cpp
  dds/DCPS/RTPS/ParameterListConverter.cpp
  dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.cpp
  dds/DCPS/RTPS/MessageUtils.cpp
  dds/DCPS/RTPS/DiscoveredEntities.cpp
  dds/DCPS/Time_Helper.cpp
  dds/DCPS/NetworkResource.cpp
  dds/DCPS/RcHandle_T.cpp
  dds/DCPS/DataSampleHeader.cpp
  dds/DCPS/DomainParticipantImpl.cpp
  dds/DCPS/AddressCache.cpp
  dds/DCPS/ServiceEventDispatcher.cpp
  dds/DCPS/security/AccessControlBuiltInImpl.cpp
  dds/DCPS/security/SSL/PrivateKey.cpp
  dds/DCPS/security/SSL/SubjectName.cpp
  dds/DCPS/security/SSL/Certificate.cpp
  dds/DCPS/security/SSL/DiffieHellman.cpp
  dds/DCPS/security/SSL/SignedDocument.cpp
  dds/DCPS/security/SSL/Utils.cpp
  dds/DCPS/security/Authentication/LocalAuthCredentialData.cpp
  dds/DCPS/security/AccessControl/XmlUtils.cpp
  dds/DCPS/security/AccessControl/Permissions.cpp
  dds/DCPS/security/framework/SecurityConfig.cpp
  dds/DCPS/security/AuthenticationBuiltInImpl.cpp
  dds/DCPS/security/CryptoBuiltInImpl.cpp
  dds/DCPS/SafeBool_T.cpp
  dds/DCPS/XTypes/DynamicDataImpl.cpp
  dds/DCPS/XTypes/MemberDescriptorImpl.cpp
  dds/DCPS/XTypes/IdlScanner.cpp
  dds/DCPS/XTypes/DynamicTypeImpl.cpp
  dds/DCPS/XTypes/TypeAssignability.cpp
  dds/DCPS/XTypes/TypeObject.cpp
  dds/DCPS/XTypes/DynamicDataXcdrReadImpl.cpp
  dds/DCPS/XTypes/Utils.cpp
  dds/DCPS/XTypes/DynamicDataAdapter.cpp
  dds/DCPS/EventDispatcher.cpp
  dds/DCPS/SafetyProfilePool.cpp
  dds/DCPS/TimeDuration.cpp
  dds/DCPS/InternalTopic.cpp
  dds/DCPS/SequenceNumber.cpp
  dds/DCPS/JsonValueWriter.cpp
  dds/DdsDcpsCoreTypeSupportImpl.cpp
  dds/FACE/config/QosSettings.cpp
  FACE/Sequence.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME UnitTests
  UNITY_BUILD FALSE # TODO
)
_opendds_msvc_bigobj(${PROJECT_NAME})
opendds_target_sources(${PROJECT_NAME}
  PUBLIC
    dds/DCPS/XTypes/CompleteToDynamicType.idl
    dds/DCPS/XTypes/CompleteToMinimalTypeObject.idl
    dds/DCPS/XTypes/XTypesUtils.idl
    dds/DCPS/XTypes/DynamicDataAdapter.idl
  INCLUDE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
  TAO_IDL_OPTIONS -SS
  OPENDDS_IDL_OPTIONS -Gxtypes-complete
)
opendds_target_sources(${PROJECT_NAME}
  PUBLIC
    dds/DCPS/XTypes/DynamicDataXcdrReadImpl.idl
    dds/DCPS/XTypes/DynamicDataImpl.idl
  INCLUDE_BASE "${CMAKE_CURRENT_SOURCE_DIR}"
  TAO_IDL_OPTIONS -SS
  OPENDDS_IDL_OPTIONS -Gxtypes-complete -SI -Sv
)
opendds_target_sources(${PROJECT_NAME}
  PUBLIC
    ../DCPS/Compiler/key_annotation/key_annotation.idl
  TAO_IDL_OPTIONS -SS
  OPENDDS_IDL_OPTIONS -Gxtypes-complete
)
target_link_libraries(${PROJECT_NAME} PUBLIC
  OpenDDS::Dcps
  OpenDDS::Rtps_Udp
  OpenDDS_GoogleTest
  gmock
)
if(TARGET OpenDDS::Security)
  target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Security)
endif()

opendds_add_test()
