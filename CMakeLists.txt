cmake_minimum_required(VERSION 3.22)

project(opendds CXX)
enable_testing()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FetchContent)

set(OPENDDS_IS_BEING_BUILT TRUE CACHE INTERNAL "")
set(OPENDDS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")
set(OPENDDS_BIN_DIR "${OPENDDS_BUILD_DIR}/bin" CACHE INTERNAL "")
set(OPENDDS_LIB_DIR "${OPENDDS_BUILD_DIR}/lib" CACHE INTERNAL "")
set(OPENDDS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
list(APPEND CMAKE_PREFIX_PATH "${OPENDDS_SOURCE_DIR}")
list(APPEND CMAKE_FIND_ROOT_PATH "${OPENDDS_SOURCE_DIR}")
set(_OPENDDS_CMAKE_DIR "${OPENDDS_SOURCE_DIR}/cmake" CACHE INTERNAL "")

include("${_OPENDDS_CMAKE_DIR}/opendds_features.cmake")

if(NOT DEFINED OPENDDS_ACE)
  include("${_OPENDDS_CMAKE_DIR}/build_ace_tao.cmake")
endif()

if(DEFINED OPENDDS_ACE_TAO_HOST_TOOLS)
  set(_OPENDDS_ACE_HOST_TOOLS "${OPENDDS_ACE_TAO_HOST_TOOLS}/bin" CACHE INTERNAL "")
  set(_OPENDDS_TAO_HOST_TOOLS "${OPENDDS_ACE_TAO_HOST_TOOLS}/bin" CACHE INTERNAL "")
endif()
if(DEFINED OPENDDS_HOST_TOOLS)
  set(_OPENDDS_OPENDDS_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/bin" CACHE INTERNAL "")
endif()

if(NOT DEFINED OPENDDS_RAPIDJSON)
  set(OPENDDS_RAPIDJSON "${OPENDDS_SOURCE_DIR}/tools/rapidjson")
  if(NOT EXISTS "${OPENDDS_RAPIDJSON}/include")
    FetchContent_Declare(rapidjson
      GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
      GIT_TAG fd3dc29a5c2852df569e1ea81dbde2c412ac5051
    )
    FetchContent_Populate(rapidjson)
    set(OPENDDS_RAPIDJSON "${rapidjson_SOURCE_DIR}")
  endif()
endif()

if(NOT CMAKE_CROSSCOMPILING)
  add_subdirectory(dds/DCPS)
  add_subdirectory(dds/idl)
  if(BUILD_OPENDDS_HOST_TOOLS)
    return()
  endif()
endif()

# TODO: Move Dcps to dds/CMakeLists.txt?

set(dcps_lib_deps
  ACE::ACE
  TAO::TAO
  TAO::PortableServer
  TAO::BiDirGIOP
  TAO::Valuetype
)
find_package(OpenDDS REQUIRED NO_DEFAULTS TAO::tao_idl OpenDDS::opendds_idl ${dcps_lib_deps})
include(opendds_build_helpers)

add_library(OpenDDS_Dcps
  dds/DCPS/BitPubListenerImpl.cpp
  dds/DCPS/BuiltInTopicUtils.cpp
  dds/DCPS/CoherentChangeControl.cpp
  dds/DCPS/ConditionImpl.cpp
  dds/DCPS/ConfigStoreImpl.cpp
  dds/DCPS/ConfigUtils.cpp
  dds/DCPS/ConnectionRecords.cpp
  dds/DCPS/ContentFilteredTopicImpl.cpp
  dds/DCPS/DCPS_Utils.cpp
  dds/DCPS/DataDurabilityCache.cpp
  dds/DCPS/DataReaderImpl.cpp
  dds/DCPS/DataSampleElement.cpp
  dds/DCPS/DataSampleHeader.cpp
  dds/DCPS/DataWriterImpl.cpp
  dds/DCPS/DcpsUpcalls.cpp
  dds/DCPS/DdsDcps_pch.cpp
  dds/DCPS/Discovery.cpp
  dds/DCPS/DisjointSequence.cpp
  dds/DCPS/DispatchService.cpp
  dds/DCPS/DomainParticipantFactoryImpl.cpp
  dds/DCPS/DomainParticipantImpl.cpp
  dds/DCPS/EntityImpl.cpp
  dds/DCPS/EventDispatcher.cpp
  dds/DCPS/FileSystemStorage.cpp
  dds/DCPS/FilterEvaluator.cpp
  dds/DCPS/GroupRakeData.cpp
  dds/DCPS/GuardCondition.cpp
  dds/DCPS/GuidBuilder.cpp
  dds/DCPS/GuidConverter.cpp
  dds/DCPS/GuidUtils.cpp
  dds/DCPS/Hash.cpp
  dds/DCPS/InstanceDataSampleList.cpp
  dds/DCPS/InstanceHandle.cpp
  dds/DCPS/InstanceState.cpp
  dds/DCPS/JobQueue.cpp
  dds/DCPS/LinuxNetworkConfigMonitor.cpp
  dds/DCPS/LogAddr.cpp
  dds/DCPS/Logging.cpp
  dds/DCPS/Marked_Default_Qos.cpp
  dds/DCPS/MemoryPool.cpp
  dds/DCPS/MessageBlock.cpp
  dds/DCPS/MessageTracker.cpp
  dds/DCPS/MonitorFactory.cpp
  dds/DCPS/MultiTopicDataReaderBase.cpp
  dds/DCPS/MultiTopicImpl.cpp
  dds/DCPS/MulticastManager.cpp
  dds/DCPS/NetworkAddress.cpp
  dds/DCPS/NetworkConfigModifier.cpp
  dds/DCPS/NetworkConfigMonitor.cpp
  dds/DCPS/NetworkResource.cpp
  dds/DCPS/Observer.cpp
  dds/DCPS/OwnershipManager.cpp
  dds/DCPS/PeriodicEvent.cpp
  dds/DCPS/PublisherImpl.cpp
  dds/DCPS/Qos_Helper.cpp
  dds/DCPS/QueryConditionImpl.cpp
  dds/DCPS/RawDataSample.cpp
  dds/DCPS/ReactorInterceptor.cpp
  dds/DCPS/ReactorTask.cpp
  dds/DCPS/ReadConditionImpl.cpp
  dds/DCPS/ReceivedDataElementList.cpp
  dds/DCPS/ReceivedDataStrategy.cpp
  dds/DCPS/Recorder.cpp
  dds/DCPS/RecorderImpl.cpp
  dds/DCPS/Registered_Data_Types.cpp
  dds/DCPS/Replayer.cpp
  dds/DCPS/ReplayerImpl.cpp
  dds/DCPS/RepoIdBuilder.cpp
  dds/DCPS/RepoIdConverter.cpp
  dds/DCPS/RepoIdGenerator.cpp
  dds/DCPS/SafetyProfilePool.cpp
  dds/DCPS/SafetyProfileSequences.cpp
  dds/DCPS/SafetyProfileStreams.cpp
  dds/DCPS/SendStateDataSampleList.cpp
  dds/DCPS/SequenceNumber.cpp
  dds/DCPS/Serializer.cpp
  dds/DCPS/ServiceEventDispatcher.cpp
  dds/DCPS/Service_Participant.cpp
  dds/DCPS/SporadicEvent.cpp
  dds/DCPS/StaticDiscovery.cpp
  dds/DCPS/StatusConditionImpl.cpp
  dds/DCPS/SubscriberImpl.cpp
  dds/DCPS/SubscriptionInstance.cpp
  dds/DCPS/ThreadPool.cpp
  dds/DCPS/ThreadStatusManager.cpp
  dds/DCPS/TimeDuration.cpp
  dds/DCPS/Time_Helper.cpp
  dds/DCPS/TopicDescriptionImpl.cpp
  dds/DCPS/TopicImpl.cpp
  dds/DCPS/Transient_Kludge.cpp
  dds/DCPS/TypeSupportImpl.cpp
  dds/DCPS/ValueReader.cpp
  dds/DCPS/ValueWriter.cpp
  dds/DCPS/WaitSet.cpp
  dds/DCPS/WriteDataContainer.cpp
  dds/DCPS/WriterDataSampleList.cpp
  dds/DCPS/WriterInfo.cpp
  dds/DCPS/XTypes/DynamicDataAdapter.cpp
  dds/DCPS/XTypes/DynamicDataBase.cpp
  dds/DCPS/XTypes/DynamicDataFactory.cpp
  dds/DCPS/XTypes/DynamicDataImpl.cpp
  dds/DCPS/XTypes/DynamicDataReaderImpl.cpp
  dds/DCPS/XTypes/DynamicDataXcdrReadImpl.cpp
  dds/DCPS/XTypes/DynamicSample.cpp
  dds/DCPS/XTypes/DynamicTypeImpl.cpp
  dds/DCPS/XTypes/DynamicTypeMemberImpl.cpp
  dds/DCPS/XTypes/DynamicTypeSupport.cpp
  dds/DCPS/XTypes/MemberDescriptorImpl.cpp
  dds/DCPS/XTypes/TypeAssignability.cpp
  dds/DCPS/XTypes/TypeDescriptorImpl.cpp
  dds/DCPS/XTypes/TypeLookupService.cpp
  dds/DCPS/XTypes/TypeObject.cpp
  dds/DCPS/XTypes/Utils.cpp
  dds/DCPS/debug.cpp
  dds/DCPS/security/framework/HandleRegistry.cpp
  dds/DCPS/security/framework/SecurityConfig.cpp
  dds/DCPS/security/framework/SecurityPluginInst.cpp
  dds/DCPS/security/framework/SecurityRegistry.cpp
  dds/DCPS/transport/framework/BuildChainVisitor.cpp
  dds/DCPS/transport/framework/CopyChainVisitor.cpp
  dds/DCPS/transport/framework/DataLink.cpp
  dds/DCPS/transport/framework/DataLinkCleanupTask.cpp
  dds/DCPS/transport/framework/DataLinkSet.cpp
  dds/DCPS/transport/framework/DirectPriorityMapper.cpp
  dds/DCPS/transport/framework/MessageDropper.cpp
  dds/DCPS/transport/framework/NullSynch.cpp
  dds/DCPS/transport/framework/NullSynchStrategy.cpp
  dds/DCPS/transport/framework/PacketRemoveVisitor.cpp
  dds/DCPS/transport/framework/PerConnectionSynch.cpp
  dds/DCPS/transport/framework/PerConnectionSynchStrategy.cpp
  dds/DCPS/transport/framework/PoolSynch.cpp
  dds/DCPS/transport/framework/PoolSynchStrategy.cpp
  dds/DCPS/transport/framework/PriorityKey.cpp
  dds/DCPS/transport/framework/PriorityMapper.cpp
  dds/DCPS/transport/framework/QueueRemoveVisitor.cpp
  dds/DCPS/transport/framework/ReactorSynch.cpp
  dds/DCPS/transport/framework/ReactorSynchStrategy.cpp
  dds/DCPS/transport/framework/ReceiveListenerSet.cpp
  dds/DCPS/transport/framework/ReceiveListenerSetMap.cpp
  dds/DCPS/transport/framework/ReceivedDataSample.cpp
  dds/DCPS/transport/framework/RemoveAllVisitor.cpp
  dds/DCPS/transport/framework/ScheduleOutputHandler.cpp
  dds/DCPS/transport/framework/SendResponseListener.cpp
  dds/DCPS/transport/framework/ThreadPerConRemoveVisitor.cpp
  dds/DCPS/transport/framework/ThreadPerConnectionSendTask.cpp
  dds/DCPS/transport/framework/ThreadSynch.cpp
  dds/DCPS/transport/framework/ThreadSynchResource.cpp
  dds/DCPS/transport/framework/ThreadSynchStrategy.cpp
  dds/DCPS/transport/framework/ThreadSynchWorker.cpp
  dds/DCPS/transport/framework/TransportClient.cpp
  dds/DCPS/transport/framework/TransportConfig.cpp
  dds/DCPS/transport/framework/TransportControlElement.cpp
  dds/DCPS/transport/framework/TransportCustomizedElement.cpp
  dds/DCPS/transport/framework/TransportDebug.cpp
  dds/DCPS/transport/framework/TransportHeader.cpp
  dds/DCPS/transport/framework/TransportImpl.cpp
  dds/DCPS/transport/framework/TransportInst.cpp
  dds/DCPS/transport/framework/TransportQueueElement.cpp
  dds/DCPS/transport/framework/TransportReassembly.cpp
  dds/DCPS/transport/framework/TransportReceiveListener.cpp
  dds/DCPS/transport/framework/TransportReceiveStrategy.cpp
  dds/DCPS/transport/framework/TransportRegistry.cpp
  dds/DCPS/transport/framework/TransportReplacedElement.cpp
  dds/DCPS/transport/framework/TransportRetainedElement.cpp
  dds/DCPS/transport/framework/TransportSendBuffer.cpp
  dds/DCPS/transport/framework/TransportSendControlElement.cpp
  dds/DCPS/transport/framework/TransportSendElement.cpp
  dds/DCPS/transport/framework/TransportSendListener.cpp
  dds/DCPS/transport/framework/TransportSendStrategy.cpp
  dds/DCPS/transport/framework/TransportStatistics.cpp
  dds/DCPS/transport/framework/TransportStrategy.cpp
  dds/DCPS/transport/framework/TransportType.cpp
)
target_sources(OpenDDS_Dcps
  PUBLIC FILE_SET HEADERS FILES
    FACE/Fixed.h
    FACE/OpenDDS_FACE_Export.h
    FACE/Sequence.h
    FACE/SequenceVar.h
    FACE/StringManager.h
    FACE/TS.hpp
    FACE/TS_common.hpp
    FACE/common.hpp
    FACE/types.hpp
    dds/DCPS/AddressCache.h
    dds/DCPS/AssociationData.h
    dds/DCPS/AstNodeWrapper.h
    dds/DCPS/Atomic.h
    dds/DCPS/AtomicBool.h
    dds/DCPS/BitPubListenerImpl.h
    dds/DCPS/BuiltInTopicDataReaderImpls.h
    dds/DCPS/BuiltInTopicUtils.h
    dds/DCPS/Cached_Allocator_With_Overflow_T.h
    dds/DCPS/CoherentChangeControl.h
    dds/DCPS/CoherentChangeControl.inl
    dds/DCPS/Comparator_T.h
    dds/DCPS/ConditionImpl.h
    dds/DCPS/ConditionVariable.h
    dds/DCPS/ConfigStoreImpl.h
    dds/DCPS/ConfigUtils.h
    dds/DCPS/ConnectionRecords.h
    dds/DCPS/ContentFilteredTopicImpl.h
    dds/DCPS/DCPS_Utils.h
    dds/DCPS/DataBlockLockPool.h
    dds/DCPS/DataCollector_T.cpp
    dds/DCPS/DataCollector_T.h
    dds/DCPS/DataCollector_T.inl
    dds/DCPS/DataDurabilityCache.h
    dds/DCPS/DataReaderCallbacks.h
    dds/DCPS/DataReaderImpl.h
    dds/DCPS/DataReaderImpl.inl
    dds/DCPS/DataReaderImpl_T.h
    dds/DCPS/DataSampleElement.h
    dds/DCPS/DataSampleElement.inl
    dds/DCPS/DataSampleHeader.h
    dds/DCPS/DataSampleHeader.inl
    dds/DCPS/DataWriterCallbacks.h
    dds/DCPS/DataWriterImpl.h
    dds/DCPS/DataWriterImpl_T.h
    dds/DCPS/DcpsUpcalls.h
    dds/DCPS/DdsDcps_pch.h
    dds/DCPS/DefaultNetworkConfigMonitor.h
    dds/DCPS/Definitions.h
    dds/DCPS/DirentWrapper.h
    dds/DCPS/Discovery.h
    dds/DCPS/DiscoveryListener.h
    dds/DCPS/DisjointSequence.h
    dds/DCPS/DisjointSequence.inl
    dds/DCPS/DispatchService.h
    dds/DCPS/DomainParticipantFactoryImpl.h
    dds/DCPS/DomainParticipantImpl.h
    dds/DCPS/DurabilityArray.h
    dds/DCPS/DurabilityQueue.h
    dds/DCPS/Dynamic_Cached_Allocator_With_Overflow_T.h
    dds/DCPS/EntityImpl.h
    dds/DCPS/EventDispatcher.h
    dds/DCPS/FeatureDisabledQosCheck.h
    dds/DCPS/FibonacciSequence.h
    dds/DCPS/FileSystemStorage.h
    dds/DCPS/FilterEvaluator.h
    dds/DCPS/FilterExpressionGrammar.h
    dds/DCPS/GroupRakeData.h
    dds/DCPS/GuardCondition.h
    dds/DCPS/GuidBuilder.h
    dds/DCPS/GuidBuilder.inl
    dds/DCPS/GuidConverter.h
    dds/DCPS/GuidUtils.h
    dds/DCPS/Hash.h
    dds/DCPS/Ice.h
    dds/DCPS/InstanceDataSampleList.h
    dds/DCPS/InstanceDataSampleList.inl
    dds/DCPS/InstanceHandle.h
    dds/DCPS/InstanceState.h
    dds/DCPS/InstanceState.inl
    dds/DCPS/InternalDataReader.h
    dds/DCPS/InternalDataReaderListener.h
    dds/DCPS/InternalDataWriter.h
    dds/DCPS/InternalTopic.h
    dds/DCPS/JobQueue.h
    dds/DCPS/JsonValueReader.h
    dds/DCPS/JsonValueWriter.h
    dds/DCPS/LinuxNetworkConfigMonitor.h
    dds/DCPS/LocalObject.h
    dds/DCPS/LogAddr.h
    dds/DCPS/Logging.h
    dds/DCPS/Marked_Default_Qos.h
    dds/DCPS/MemoryPool.h
    dds/DCPS/MessageBlock.h
    dds/DCPS/MessageTracker.h
    dds/DCPS/Message_Block_Ptr.h
    dds/DCPS/MonitorFactory.h
    dds/DCPS/MultiTask.h
    dds/DCPS/MultiTopicDataReaderBase.h
    dds/DCPS/MultiTopicDataReader_T.cpp
    dds/DCPS/MultiTopicDataReader_T.h
    dds/DCPS/MultiTopicImpl.h
    dds/DCPS/MulticastManager.h
    dds/DCPS/NetworkAddress.h
    dds/DCPS/NetworkConfigModifier.h
    dds/DCPS/NetworkConfigMonitor.h
    dds/DCPS/NetworkResource.h
    dds/DCPS/NetworkResource.inl
    dds/DCPS/Observer.h
    dds/DCPS/OwnershipManager.h
    dds/DCPS/PeriodicEvent.h
    dds/DCPS/PeriodicTask.h
    dds/DCPS/PoolAllocationBase.h
    dds/DCPS/PoolAllocator.h
    dds/DCPS/PrinterValueWriter.h
    dds/DCPS/PublicationInstance.h
    dds/DCPS/PublisherImpl.h
    dds/DCPS/Qos_Helper.h
    dds/DCPS/Qos_Helper.inl
    dds/DCPS/QueryConditionImpl.h
    dds/DCPS/RakeData.h
    dds/DCPS/RakeResults_T.cpp
    dds/DCPS/RakeResults_T.h
    dds/DCPS/RapidJsonWrapper.h
    dds/DCPS/RawDataSample.h
    dds/DCPS/RcEventHandler.h
    dds/DCPS/RcHandle_T.h
    dds/DCPS/RcObject.h
    dds/DCPS/ReactorInterceptor.h
    dds/DCPS/ReactorTask.h
    dds/DCPS/ReactorTask.inl
    dds/DCPS/ReactorTask_rch.h
    dds/DCPS/ReadConditionImpl.h
    dds/DCPS/ReceivedDataElementList.h
    dds/DCPS/ReceivedDataElementList.inl
    dds/DCPS/ReceivedDataStrategy.h
    dds/DCPS/Recorder.h
    dds/DCPS/RecorderImpl.h
    dds/DCPS/Registered_Data_Types.h
    dds/DCPS/Replayer.h
    dds/DCPS/ReplayerImpl.h
    dds/DCPS/RepoIdBuilder.h
    dds/DCPS/RepoIdBuilder.inl
    dds/DCPS/RepoIdConverter.h
    dds/DCPS/RepoIdGenerator.h
    dds/DCPS/RestoreOutputStreamState.h
    dds/DCPS/SafeBool_T.h
    dds/DCPS/SafetyProfilePool.h
    dds/DCPS/SafetyProfileSequence.h
    dds/DCPS/SafetyProfileSequenceVar.h
    dds/DCPS/SafetyProfileSequences.h
    dds/DCPS/SafetyProfileStreams.h
    dds/DCPS/Sample.h
    dds/DCPS/SendStateDataSampleList.h
    dds/DCPS/SendStateDataSampleList.inl
    dds/DCPS/SequenceIterator.h
    dds/DCPS/SequenceNumber.h
    dds/DCPS/Serializer.h
    dds/DCPS/Serializer.inl
    dds/DCPS/ServiceEventDispatcher.h
    dds/DCPS/Service_Participant.h
    dds/DCPS/Service_Participant.inl
    dds/DCPS/SporadicEvent.h
    dds/DCPS/SporadicTask.h
    dds/DCPS/StaticDiscovery.h
    dds/DCPS/StaticIncludes.h
    dds/DCPS/Stats_T.h
    dds/DCPS/StatusConditionImpl.h
    dds/DCPS/SubscriberImpl.h
    dds/DCPS/SubscriptionInstance.h
    dds/DCPS/ThreadPool.h
    dds/DCPS/ThreadStatusManager.h
    dds/DCPS/TimeDuration.h
    dds/DCPS/TimeDuration.inl
    dds/DCPS/TimePoint_T.cpp
    dds/DCPS/TimePoint_T.h
    dds/DCPS/TimePoint_T.inl
    dds/DCPS/TimeSource.h
    dds/DCPS/TimeTypes.h
    dds/DCPS/Time_Helper.h
    dds/DCPS/Time_Helper.inl
    dds/DCPS/TopicCallbacks.h
    dds/DCPS/TopicDescriptionImpl.h
    dds/DCPS/TopicDetails.h
    dds/DCPS/TopicExpressionGrammar.h
    dds/DCPS/TopicImpl.h
    dds/DCPS/Transient_Kludge.h
    dds/DCPS/Transient_Kludge.inl
    dds/DCPS/TypeSupportImpl.h
    dds/DCPS/Util.h
    dds/DCPS/ValueDispatcher.h
    dds/DCPS/ValueHelper.h
    dds/DCPS/ValueReader.h
    dds/DCPS/ValueWriter.h
    dds/DCPS/WaitSet.h
    dds/DCPS/WriteDataContainer.h
    dds/DCPS/WriterDataSampleList.h
    dds/DCPS/WriterDataSampleList.inl
    dds/DCPS/WriterInfo.h
    dds/DCPS/XTypes/DynamicDataAdapter.h
    dds/DCPS/XTypes/DynamicDataBase.h
    dds/DCPS/XTypes/DynamicDataFactory.h
    dds/DCPS/XTypes/DynamicDataImpl.h
    dds/DCPS/XTypes/DynamicDataReaderImpl.h
    dds/DCPS/XTypes/DynamicDataWriterImpl.h
    dds/DCPS/XTypes/DynamicDataXcdrReadImpl.h
    dds/DCPS/XTypes/DynamicSample.h
    dds/DCPS/XTypes/DynamicTypeImpl.h
    dds/DCPS/XTypes/DynamicTypeMemberImpl.h
    dds/DCPS/XTypes/DynamicTypeSupport.h
    dds/DCPS/XTypes/External.h
    dds/DCPS/XTypes/MemberDescriptorImpl.h
    dds/DCPS/XTypes/TypeAssignability.h
    dds/DCPS/XTypes/TypeDescriptorImpl.h
    dds/DCPS/XTypes/TypeLookupService.h
    dds/DCPS/XTypes/TypeObject.h
    dds/DCPS/XTypes/TypeObjectC.h
    dds/DCPS/XTypes/TypeObjectTypeSupportImpl.h
    dds/DCPS/XTypes/Utils.h
    dds/DCPS/ZeroCopyAllocator_T.cpp
    dds/DCPS/ZeroCopyAllocator_T.h
    dds/DCPS/ZeroCopyAllocator_T.inl
    dds/DCPS/ZeroCopyInfoSeq_T.h
    dds/DCPS/ZeroCopySeqBase.h
    dds/DCPS/ZeroCopySeq_T.cpp
    dds/DCPS/ZeroCopySeq_T.h
    dds/DCPS/ZeroCopySeq_T.inl
    dds/DCPS/dcps_export.h
    dds/DCPS/debug.h
    dds/DCPS/security/framework/HandleRegistry.h
    dds/DCPS/security/framework/Properties.h
    dds/DCPS/security/framework/SecurityConfig.h
    dds/DCPS/security/framework/SecurityConfigPropertyList.h
    dds/DCPS/security/framework/SecurityConfig_rch.h
    dds/DCPS/security/framework/SecurityPluginInst.h
    dds/DCPS/security/framework/SecurityPluginInst_rch.h
    dds/DCPS/security/framework/SecurityRegistry.h
    dds/DCPS/transport/framework/BasicQueueVisitor_T.h
    dds/DCPS/transport/framework/BasicQueue_T.h
    dds/DCPS/transport/framework/BuildChainVisitor.h
    dds/DCPS/transport/framework/BuildChainVisitor.inl
    dds/DCPS/transport/framework/CopyChainVisitor.h
    dds/DCPS/transport/framework/CopyChainVisitor.inl
    dds/DCPS/transport/framework/DataLink.h
    dds/DCPS/transport/framework/DataLink.inl
    dds/DCPS/transport/framework/DataLinkCleanupTask.h
    dds/DCPS/transport/framework/DataLinkSet.h
    dds/DCPS/transport/framework/DataLinkSet.inl
    dds/DCPS/transport/framework/DataLinkSet_rch.h
    dds/DCPS/transport/framework/DataLink_rch.h
    dds/DCPS/transport/framework/DirectPriorityMapper.h
    dds/DCPS/transport/framework/DirectPriorityMapper.inl
    dds/DCPS/transport/framework/EntryExit.h
    dds/DCPS/transport/framework/MessageDropper.h
    dds/DCPS/transport/framework/NullSynch.h
    dds/DCPS/transport/framework/NullSynch.inl
    dds/DCPS/transport/framework/NullSynchStrategy.h
    dds/DCPS/transport/framework/PacketRemoveVisitor.h
    dds/DCPS/transport/framework/PacketRemoveVisitor.inl
    dds/DCPS/transport/framework/PerConnectionSynch.h
    dds/DCPS/transport/framework/PerConnectionSynch.inl
    dds/DCPS/transport/framework/PerConnectionSynchStrategy.h
    dds/DCPS/transport/framework/PerConnectionSynchStrategy.inl
    dds/DCPS/transport/framework/PoolSynch.h
    dds/DCPS/transport/framework/PoolSynch.inl
    dds/DCPS/transport/framework/PoolSynchStrategy.h
    dds/DCPS/transport/framework/PoolSynchStrategy.inl
    dds/DCPS/transport/framework/PriorityKey.h
    dds/DCPS/transport/framework/PriorityKey.inl
    dds/DCPS/transport/framework/PriorityMapper.h
    dds/DCPS/transport/framework/PriorityMapper.inl
    dds/DCPS/transport/framework/QueueRemoveVisitor.h
    dds/DCPS/transport/framework/QueueRemoveVisitor.inl
    dds/DCPS/transport/framework/QueueTaskBase_T.h
    dds/DCPS/transport/framework/ReactorSynch.h
    dds/DCPS/transport/framework/ReactorSynch.inl
    dds/DCPS/transport/framework/ReactorSynchStrategy.h
    dds/DCPS/transport/framework/ReactorSynchStrategy.inl
    dds/DCPS/transport/framework/ReceiveListenerSet.h
    dds/DCPS/transport/framework/ReceiveListenerSet.inl
    dds/DCPS/transport/framework/ReceiveListenerSetMap.h
    dds/DCPS/transport/framework/ReceiveListenerSetMap.inl
    dds/DCPS/transport/framework/ReceiveListenerSet_rch.h
    dds/DCPS/transport/framework/ReceivedDataSample.h
    dds/DCPS/transport/framework/RemoveAllVisitor.h
    dds/DCPS/transport/framework/RemoveAllVisitor.inl
    dds/DCPS/transport/framework/ScheduleOutputHandler.h
    dds/DCPS/transport/framework/ScheduleOutputHandler.inl
    dds/DCPS/transport/framework/SendResponseListener.h
    dds/DCPS/transport/framework/ThreadPerConRemoveVisitor.h
    dds/DCPS/transport/framework/ThreadPerConRemoveVisitor.inl
    dds/DCPS/transport/framework/ThreadPerConnectionSendTask.h
    dds/DCPS/transport/framework/ThreadSynch.h
    dds/DCPS/transport/framework/ThreadSynch.inl
    dds/DCPS/transport/framework/ThreadSynchResource.h
    dds/DCPS/transport/framework/ThreadSynchResource.inl
    dds/DCPS/transport/framework/ThreadSynchStrategy.h
    dds/DCPS/transport/framework/ThreadSynchStrategy.inl
    dds/DCPS/transport/framework/ThreadSynchStrategy_rch.h
    dds/DCPS/transport/framework/ThreadSynchWorker.h
    dds/DCPS/transport/framework/ThreadSynchWorker.inl
    dds/DCPS/transport/framework/TransportClient.h
    dds/DCPS/transport/framework/TransportConfig.h
    dds/DCPS/transport/framework/TransportConfig_rch.h
    dds/DCPS/transport/framework/TransportControlElement.h
    dds/DCPS/transport/framework/TransportControlElement.inl
    dds/DCPS/transport/framework/TransportCustomizedElement.h
    dds/DCPS/transport/framework/TransportCustomizedElement.inl
    dds/DCPS/transport/framework/TransportDebug.h
    dds/DCPS/transport/framework/TransportDefs.h
    dds/DCPS/transport/framework/TransportExceptions.h
    dds/DCPS/transport/framework/TransportHeader.h
    dds/DCPS/transport/framework/TransportHeader.inl
    dds/DCPS/transport/framework/TransportImpl.h
    dds/DCPS/transport/framework/TransportImpl.inl
    dds/DCPS/transport/framework/TransportImpl_rch.h
    dds/DCPS/transport/framework/TransportInst.h
    dds/DCPS/transport/framework/TransportInst.inl
    dds/DCPS/transport/framework/TransportInst_rch.h
    dds/DCPS/transport/framework/TransportQueueElement.h
    dds/DCPS/transport/framework/TransportQueueElement.inl
    dds/DCPS/transport/framework/TransportReassembly.h
    dds/DCPS/transport/framework/TransportReceiveListener.h
    dds/DCPS/transport/framework/TransportReceiveStrategy_T.cpp
    dds/DCPS/transport/framework/TransportReceiveStrategy_T.h
    dds/DCPS/transport/framework/TransportReceiveStrategy_T.inl
    dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h
    dds/DCPS/transport/framework/TransportRegistry.h
    dds/DCPS/transport/framework/TransportRegistry.inl
    dds/DCPS/transport/framework/TransportReplacedElement.h
    dds/DCPS/transport/framework/TransportReplacedElement.inl
    dds/DCPS/transport/framework/TransportRetainedElement.h
    dds/DCPS/transport/framework/TransportRetainedElement.inl
    dds/DCPS/transport/framework/TransportSendBuffer.h
    dds/DCPS/transport/framework/TransportSendBuffer.inl
    dds/DCPS/transport/framework/TransportSendControlElement.h
    dds/DCPS/transport/framework/TransportSendControlElement.inl
    dds/DCPS/transport/framework/TransportSendElement.h
    dds/DCPS/transport/framework/TransportSendElement.inl
    dds/DCPS/transport/framework/TransportSendListener.h
    dds/DCPS/transport/framework/TransportSendStrategy.h
    dds/DCPS/transport/framework/TransportSendStrategy.inl
    dds/DCPS/transport/framework/TransportSendStrategy_rch.h
    dds/DCPS/transport/framework/TransportStatistics.h
    dds/DCPS/transport/framework/TransportStrategy.h
    dds/DCPS/transport/framework/TransportStrategy_rch.h
    dds/DCPS/transport/framework/TransportType.h
    dds/DCPS/transport/framework/TransportType_rch.h
    dds/DCPS/unique_ptr.h
    dds/DCPS/yard/yard_base_grammar.hpp
    dds/DCPS/yard/yard_char_set.hpp
    dds/DCPS/yard/yard_parser.hpp
    dds/DCPS/yard/yard_text_grammar.hpp
    dds/DCPS/yard/yard_tree.hpp
    dds/Version.h
    dds/Versioned_Namespace.h
)
target_compile_definitions(OpenDDS_Dcps PUBLIC ${OPENDDS_DCPS_COMPILE_DEFINITIONS})
_opendds_library(OpenDDS_Dcps MSVC_BIGOBJ)
target_include_directories(OpenDDS_Dcps
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/dds"
)
if(OPENDDS_RAPIDJSON)
  target_include_directories(OpenDDS_Dcps PUBLIC "$<BUILD_INTERFACE:${OPENDDS_RAPIDJSON}/include>")
endif()
target_link_libraries(OpenDDS_Dcps PUBLIC ${dcps_lib_deps})

set(opendds_target_sources_args
  AUTO_LINK FALSE
  USE_EXPORT "dds/DCPS/dcps_export.h" OpenDDS_Dcps_Export
  INCLUDE_BASE "${OPENDDS_SOURCE_DIR}"
)

opendds_target_sources(OpenDDS_Dcps
  PUBLIC
    dds/CorbaSeq/BooleanSeq.idl
    dds/CorbaSeq/CharSeq.idl
    dds/CorbaSeq/DoubleSeq.idl
    dds/CorbaSeq/FloatSeq.idl
    dds/CorbaSeq/Int8Seq.idl
    dds/CorbaSeq/LongDoubleSeq.idl
    dds/CorbaSeq/LongLongSeq.idl
    dds/CorbaSeq/LongSeq.idl
    dds/CorbaSeq/OctetSeq.idl
    dds/CorbaSeq/ShortSeq.idl
    dds/CorbaSeq/StringSeq.idl
    dds/CorbaSeq/UInt8Seq.idl
    dds/CorbaSeq/ULongLongSeq.idl
    dds/CorbaSeq/ULongSeq.idl
    dds/CorbaSeq/UShortSeq.idl
    dds/CorbaSeq/WCharSeq.idl
    dds/CorbaSeq/WStringSeq.idl
  SKIP_TAO_IDL
  OPENDDS_IDL_OPTIONS -SI -Wb,tao_include_prefix=tao/
  ${opendds_target_sources_args}
)

opendds_target_sources(OpenDDS_Dcps
  PUBLIC
    dds/DdsDcps.idl
    dds/DdsDcpsConditionSeq.idl # TODO
    dds/DdsDcpsCore.idl # TODO
    dds/DdsDcpsDataReaderSeq.idl # TODO
    dds/DdsDcpsDomain.idl
    dds/DdsDcpsGuid.idl # TODO
    dds/DdsDcpsInfoUtils.idl # TODO
    dds/DdsDcpsInfrastructure.idl
    dds/DdsDcpsPublication.idl
    dds/DdsDcpsSubscription.idl
    dds/DdsDcpsSubscriptionExt.idl
    dds/DdsDcpsTopic.idl
    dds/DdsDcpsTypeSupportExt.idl
    dds/DdsSecurityCore.idl # TODO
    dds/DdsSecurityParams.idl # TODO
    dds/OpenddsDcpsExt.idl # TODO
  ${opendds_target_sources_args}
)

opendds_target_sources(OpenDDS_Dcps
  PUBLIC
    dds/DdsDynamicDataSeq.idl
  ${opendds_target_sources_args}
)

# valuetypes in DdsDynamicData.idl require -Scdr
# -Scdr requires -Sa whether or not the dds_suppress_anys feature is on
opendds_target_sources(OpenDDS_Dcps
  PUBLIC
    dds/DdsDynamicData.idl
    dds/DdsDynamicTypeSupport.idl
  TAO_IDL_OPTIONS -Scdr -Sa
  ${opendds_target_sources_args}
)

get_target_property(gendir OpenDDS_Dcps OPENDDS_GENERATED_DIRECTORY)
set(config_file "${gendir}/dds/DCPS/config.h")
configure_file("${_OPENDDS_CMAKE_DIR}/config.h.in" "${config_file}")
opendds_install_interface_files(OpenDDS_Dcps INCLUDE_BASE "${OPENDDS_SOURCE_DIR}"
  EXTRA_GENERATED_FILES "${config_file}"
)

add_subdirectory(dds/DCPS/transport/multicast)
add_subdirectory(dds/DCPS/RTPS)
add_subdirectory(dds/DCPS/transport/rtps_udp)
add_subdirectory(dds/DCPS/transport/shmem)
add_subdirectory(dds/DCPS/transport/tcp)
add_subdirectory(dds/DCPS/InfoRepoDiscovery)
add_subdirectory(dds/DCPS/transport/udp)
add_subdirectory(dds/InfoRepo)
if(OPENDDS_SECURITY)
  add_subdirectory(dds/DCPS/security)
endif()
add_subdirectory(tools/inspect)
add_subdirectory(tools/dds/rtpsrelaylib)
add_subdirectory(tools/rtpsrelay)

set(cmake_dest "${CMAKE_INSTALL_DATAROOTDIR}/cmake/OpenDDS")
install(EXPORT opendds_targets
  FILE "opendds_targets.cmake"
  DESTINATION ${cmake_dest}
)
set(pkg_config_cmake "opendds_installed_config.cmake")
set(tmp_pkg_config_cmake "${CMAKE_CURRENT_BINARY_DIR}/${pkg_config_cmake}")
configure_package_config_file(
  "cmake/${pkg_config_cmake}.in" ${tmp_pkg_config_cmake}
  INSTALL_DESTINATION ${cmake_dest}
)
set(tmp_config_cmake "${CMAKE_CURRENT_BINARY_DIR}/config.cmake")
configure_file("${_OPENDDS_CMAKE_DIR}/config.cmake.in" "${tmp_config_cmake}")
install(
  FILES
    cmake/OpenDDSConfig.cmake
    cmake/OpenDDSConfigVersion.cmake
    cmake/ace_group.cmake
    cmake/dds_idl_sources.cmake
    cmake/export.h.in
    cmake/import_common.cmake
    cmake/init.cmake
    cmake/opendds_build_helpers.cmake
    cmake/opendds_features.cmake
    cmake/opendds_group.cmake
    cmake/opendds_target_sources.cmake
    cmake/opendds_testing.cmake
    cmake/tao_group.cmake
    cmake/tao_idl_sources.cmake
    "${tmp_pkg_config_cmake}"
    "${tmp_config_cmake}"
  DESTINATION "${cmake_dest}"
)
install(FILES VERSION.txt DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/dds/")
if(OPENDDS_RAPIDJSON)
  install(DIRECTORY "${OPENDDS_RAPIDJSON}/include/rapidjson"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

add_subdirectory(DevGuideExamples/DCPS/Messenger)
add_subdirectory(tests/cmake/ace_tao_only)
add_subdirectory(tests/cmake/Messenger)
# add_subdirectory(tests/cmake/idl_compiler_tests)
add_subdirectory(tests/cmake/include_subdir)
add_subdirectory(tests/cmake/generated_global)
