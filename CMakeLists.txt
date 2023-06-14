cmake_minimum_required(VERSION 3.22)

project(opendds)
enable_testing()

set(OPENDDS_IS_BEING_BUILT TRUE CACHE INTERNAL "")
# Need global GENERATED source file properties and need to override it for CMake tests
set(CMAKE_POLICY_DEFAULT_CMP0118 NEW CACHE INTERNAL "")
set(OPENDDS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")
set(OPENDDS_BIN_DIR "${OPENDDS_BUILD_DIR}/bin" CACHE INTERNAL "")
set(OPENDDS_LIB_DIR "${OPENDDS_BUILD_DIR}/lib" CACHE INTERNAL "")
set(OPENDDS_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
list(APPEND CMAKE_PREFIX_PATH "${OPENDDS_SOURCE_ROOT}")
set(_OPENDDS_CMAKE_DIR "${OPENDDS_SOURCE_ROOT}/cmake" CACHE INTERNAL "")
list(APPEND CMAKE_MODULE_PATH "${_OPENDDS_CMAKE_DIR}")

include("${_OPENDDS_CMAKE_DIR}/opendds_features.cmake")

if(NOT DEFINED OPENDDS_ACE)
  # TODO: Move building ACE/TAO to its own file?
  include(ExternalProject)
  include(FetchContent)

  set(ACE_IS_BEING_BUILT MPC CACHE INTERNAL "")
  set(TAO_IS_BEING_BUILT MPC CACHE INTERNAL "")
  set(OPENDDS_ACE "${OPENDDS_BUILD_DIR}/ace_tao" CACHE INTERNAL "")
  set(OPENDDS_MPC "${OPENDDS_ACE}/MPC" CACHE INTERNAL "")
  set(OPENDDS_TAO "${OPENDDS_ACE}/TAO" CACHE INTERNAL "")
  set(url_base "https://github.com/DOCGroup/ACE_TAO/releases/download/")
  set(ace_ver "7.1.0")
  set(configre_ace_tao_args)
  if(WIN32)
    # TODO
  else()
    set(ext "tar.bz2")
    set(md5 "4696c2898de3bda28dd69abf0055b7eb")
    set(mpc_type gnuace)
    if(LINUX)
      set(config_file "config-linux.h")
      list(APPEND configure_ace_tao_args --platform-macros-file "platform_linux.GNU")
    elseif(APPLE)
      # TODO
    endif()
  endif()
  if(NOT DEFINED config_file)
    message(FATAL_ERROR "Not sure how to build ACE/TAO for this system")
  endif()

  string(REPLACE "." "_" ace_ver_tag "${ace_ver}")
  FetchContent_Declare(ace_tao_dl
    PREFIX "${OPENDDS_BUILD_DIR}/ace_tao_tmp"
    SOURCE_DIR "${OPENDDS_ACE}"
    URL "${url_base}ACE+TAO-${ace_ver_tag}/ACE+TAO-src-${ace_ver}.${ext}"
    URL_MD5 "${md5}"
  )
  FetchContent_Populate(ace_tao_dl)

  find_package(Perl REQUIRED)
  execute_process(
    COMMAND
      "${PERL_EXECUTABLE}" "${_OPENDDS_CMAKE_DIR}/configure_ace_tao.pl"
      --mpc "${OPENDDS_MPC}"
      --mpc-type "${mpc_type}"
      --ace "${OPENDDS_ACE}"
      --tao "${OPENDDS_TAO}"
      ${configure_ace_tao_args}
      --config-file "${config_file}"
      ${_OPENDDS_MPC_FEATURES}
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
  )

  execute_process(
    COMMAND
      "${PERL_EXECUTABLE}" "${_OPENDDS_CMAKE_DIR}/scrap_gnuace.pl"
      --workspace "${OPENDDS_ACE}"
      --loc-base "${OPENDDS_BUILD_DIR}"
      --ace "${OPENDDS_ACE}"
      --tao "${OPENDDS_TAO}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_VARIABLE mpc_projects
  )
  set(_OPENDDS_ACE_MPC_PROJECTS "${mpc_projects}" CACHE INTERNAL "")
  set(_OPENDDS_ACE_MPC_EXTERNAL_PROJECT "build_ace_tao" CACHE INTERNAL "")
  set(_OPENDDS_TAO_MPC_PROJECTS "${mpc_projects}" CACHE INTERNAL "")
  set(_OPENDDS_TAO_MPC_EXTERNAL_PROJECT "build_ace_tao" CACHE INTERNAL "")

  string(JSON project_count LENGTH "${mpc_projects}")
  if(project_count EQUAL 0)
    message(FATAL_ERROR "MPC projects was empty!")
  endif()
  set(byproducts)
  math(EXPR member_index_end "${project_count} - 1")
  foreach(member_index RANGE ${member_index_end})
    string(JSON member_name MEMBER "${mpc_projects}" ${member_index})
    string(JSON mpc_project GET "${mpc_projects}" ${member_name})
    string(JSON file GET "${mpc_project}" loc)
    list(APPEND byproducts "${file}")
  endforeach()

  ExternalProject_Add(build_ace_tao
    SOURCE_DIR "${OPENDDS_ACE}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "Already configured"
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND
      "${CMAKE_COMMAND}" -E env "ACE_ROOT=${OPENDDS_ACE}" "TAO_ROOT=${OPENDDS_TAO}"
      make -j8
    BUILD_BYPRODUCTS ${byproducts}
    USES_TERMINAL_BUILD TRUE # Needed for Ninja to show the ACE/TAO build
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "No install step"
  )
endif()

add_subdirectory(dds/DCPS)
add_subdirectory(dds/idl)

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
  dds/DCPS/CoherentChangeControl.inl
  dds/DCPS/ConditionImpl.cpp
  dds/DCPS/ConfigStoreImpl.cpp
  dds/DCPS/ConfigUtils.cpp
  dds/DCPS/ConnectionRecords.cpp
  dds/DCPS/ContentFilteredTopicImpl.cpp
  dds/DCPS/DCPS_Utils.cpp
  # dds/DCPS/DataCollector_T.cpp
  dds/DCPS/DataCollector_T.inl
  dds/DCPS/DataDurabilityCache.cpp
  dds/DCPS/DataReaderImpl.cpp
  dds/DCPS/DataReaderImpl.inl
  dds/DCPS/DataSampleElement.cpp
  dds/DCPS/DataSampleElement.inl
  dds/DCPS/DataSampleHeader.cpp
  dds/DCPS/DataSampleHeader.inl
  dds/DCPS/DataWriterImpl.cpp
  dds/DCPS/DcpsUpcalls.cpp
  dds/DCPS/DdsDcps_pch.cpp
  dds/DCPS/Discovery.cpp
  dds/DCPS/DisjointSequence.cpp
  dds/DCPS/DisjointSequence.inl
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
  dds/DCPS/GuidBuilder.inl
  dds/DCPS/GuidConverter.cpp
  dds/DCPS/GuidUtils.cpp
  dds/DCPS/Hash.cpp
  dds/DCPS/InstanceDataSampleList.cpp
  dds/DCPS/InstanceDataSampleList.inl
  dds/DCPS/InstanceHandle.cpp
  dds/DCPS/InstanceState.cpp
  dds/DCPS/InstanceState.inl
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
  # dds/DCPS/MultiTopicDataReader_T.cpp
  dds/DCPS/MultiTopicImpl.cpp
  dds/DCPS/MulticastManager.cpp
  dds/DCPS/NetworkAddress.cpp
  dds/DCPS/NetworkConfigModifier.cpp
  dds/DCPS/NetworkConfigMonitor.cpp
  dds/DCPS/NetworkResource.cpp
  dds/DCPS/NetworkResource.inl
  dds/DCPS/Observer.cpp
  dds/DCPS/OwnershipManager.cpp
  dds/DCPS/PeriodicEvent.cpp
  dds/DCPS/PublisherImpl.cpp
  dds/DCPS/Qos_Helper.cpp
  dds/DCPS/Qos_Helper.inl
  dds/DCPS/QueryConditionImpl.cpp
  # dds/DCPS/RakeResults_T.cpp
  dds/DCPS/RawDataSample.cpp
  dds/DCPS/ReactorInterceptor.cpp
  dds/DCPS/ReactorTask.cpp
  dds/DCPS/ReactorTask.inl
  dds/DCPS/ReadConditionImpl.cpp
  dds/DCPS/ReceivedDataElementList.cpp
  dds/DCPS/ReceivedDataElementList.inl
  dds/DCPS/ReceivedDataStrategy.cpp
  dds/DCPS/Recorder.cpp
  dds/DCPS/RecorderImpl.cpp
  dds/DCPS/Registered_Data_Types.cpp
  dds/DCPS/Replayer.cpp
  dds/DCPS/ReplayerImpl.cpp
  dds/DCPS/RepoIdBuilder.cpp
  dds/DCPS/RepoIdBuilder.inl
  dds/DCPS/RepoIdConverter.cpp
  dds/DCPS/RepoIdGenerator.cpp
  dds/DCPS/SafetyProfilePool.cpp
  dds/DCPS/SafetyProfileSequences.cpp
  dds/DCPS/SafetyProfileStreams.cpp
  dds/DCPS/SendStateDataSampleList.cpp
  dds/DCPS/SendStateDataSampleList.inl
  dds/DCPS/SequenceNumber.cpp
  dds/DCPS/Serializer.cpp
  dds/DCPS/Serializer.inl
  dds/DCPS/ServiceEventDispatcher.cpp
  dds/DCPS/Service_Participant.cpp
  dds/DCPS/Service_Participant.inl
  dds/DCPS/SporadicEvent.cpp
  dds/DCPS/StaticDiscovery.cpp
  dds/DCPS/StatusConditionImpl.cpp
  dds/DCPS/SubscriberImpl.cpp
  dds/DCPS/SubscriptionInstance.cpp
  dds/DCPS/ThreadPool.cpp
  dds/DCPS/ThreadStatusManager.cpp
  dds/DCPS/TimeDuration.cpp
  dds/DCPS/TimeDuration.inl
  # dds/DCPS/TimePoint_T.cpp
  dds/DCPS/TimePoint_T.inl
  dds/DCPS/Time_Helper.cpp
  dds/DCPS/Time_Helper.inl
  dds/DCPS/TopicDescriptionImpl.cpp
  dds/DCPS/TopicImpl.cpp
  dds/DCPS/Transient_Kludge.cpp
  dds/DCPS/Transient_Kludge.inl
  dds/DCPS/TypeSupportImpl.cpp
  dds/DCPS/ValueReader.cpp
  dds/DCPS/ValueWriter.cpp
  dds/DCPS/WaitSet.cpp
  dds/DCPS/WriteDataContainer.cpp
  dds/DCPS/WriterDataSampleList.cpp
  dds/DCPS/WriterDataSampleList.inl
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
  # dds/DCPS/ZeroCopyAllocator_T.cpp
  dds/DCPS/ZeroCopyAllocator_T.inl
  # dds/DCPS/ZeroCopySeq_T.cpp
  dds/DCPS/ZeroCopySeq_T.inl
  dds/DCPS/debug.cpp
  dds/DCPS/transport/framework/BuildChainVisitor.cpp
  dds/DCPS/transport/framework/BuildChainVisitor.inl
  dds/DCPS/transport/framework/CopyChainVisitor.cpp
  dds/DCPS/transport/framework/CopyChainVisitor.inl
  dds/DCPS/transport/framework/DataLink.cpp
  dds/DCPS/transport/framework/DataLink.inl
  dds/DCPS/transport/framework/DataLinkCleanupTask.cpp
  dds/DCPS/transport/framework/DataLinkSet.cpp
  dds/DCPS/transport/framework/DataLinkSet.inl
  dds/DCPS/transport/framework/DirectPriorityMapper.cpp
  dds/DCPS/transport/framework/DirectPriorityMapper.inl
  dds/DCPS/transport/framework/NullSynch.cpp
  dds/DCPS/transport/framework/NullSynch.inl
  dds/DCPS/transport/framework/NullSynchStrategy.cpp
  dds/DCPS/transport/framework/PacketRemoveVisitor.cpp
  dds/DCPS/transport/framework/PacketRemoveVisitor.inl
  dds/DCPS/transport/framework/PerConnectionSynch.cpp
  dds/DCPS/transport/framework/PerConnectionSynch.inl
  dds/DCPS/transport/framework/PerConnectionSynchStrategy.cpp
  dds/DCPS/transport/framework/PerConnectionSynchStrategy.inl
  dds/DCPS/transport/framework/PoolSynch.cpp
  dds/DCPS/transport/framework/PoolSynch.inl
  dds/DCPS/transport/framework/PoolSynchStrategy.cpp
  dds/DCPS/transport/framework/PoolSynchStrategy.inl
  dds/DCPS/transport/framework/PriorityKey.cpp
  dds/DCPS/transport/framework/PriorityKey.inl
  dds/DCPS/transport/framework/PriorityMapper.cpp
  dds/DCPS/transport/framework/PriorityMapper.inl
  dds/DCPS/transport/framework/QueueRemoveVisitor.cpp
  dds/DCPS/transport/framework/QueueRemoveVisitor.inl
  dds/DCPS/transport/framework/ReactorSynch.cpp
  dds/DCPS/transport/framework/ReactorSynch.inl
  dds/DCPS/transport/framework/ReactorSynchStrategy.cpp
  dds/DCPS/transport/framework/ReactorSynchStrategy.inl
  dds/DCPS/transport/framework/ReceiveListenerSet.cpp
  dds/DCPS/transport/framework/ReceiveListenerSet.inl
  dds/DCPS/transport/framework/ReceiveListenerSetMap.cpp
  dds/DCPS/transport/framework/ReceiveListenerSetMap.inl
  dds/DCPS/transport/framework/ReceivedDataSample.cpp
  dds/DCPS/transport/framework/RemoveAllVisitor.cpp
  dds/DCPS/transport/framework/RemoveAllVisitor.inl
  dds/DCPS/transport/framework/ScheduleOutputHandler.cpp
  dds/DCPS/transport/framework/ScheduleOutputHandler.inl
  dds/DCPS/transport/framework/SendResponseListener.cpp
  dds/DCPS/transport/framework/ThreadPerConRemoveVisitor.cpp
  dds/DCPS/transport/framework/ThreadPerConRemoveVisitor.inl
  dds/DCPS/transport/framework/ThreadPerConnectionSendTask.cpp
  dds/DCPS/transport/framework/ThreadSynch.cpp
  dds/DCPS/transport/framework/ThreadSynch.inl
  dds/DCPS/transport/framework/ThreadSynchResource.cpp
  dds/DCPS/transport/framework/ThreadSynchResource.inl
  dds/DCPS/transport/framework/ThreadSynchStrategy.cpp
  dds/DCPS/transport/framework/ThreadSynchStrategy.inl
  dds/DCPS/transport/framework/ThreadSynchWorker.cpp
  dds/DCPS/transport/framework/ThreadSynchWorker.inl
  dds/DCPS/transport/framework/TransportClient.cpp
  dds/DCPS/transport/framework/TransportConfig.cpp
  dds/DCPS/transport/framework/TransportControlElement.cpp
  dds/DCPS/transport/framework/TransportControlElement.inl
  dds/DCPS/transport/framework/TransportCustomizedElement.cpp
  dds/DCPS/transport/framework/TransportCustomizedElement.inl
  dds/DCPS/transport/framework/TransportDebug.cpp
  dds/DCPS/transport/framework/TransportHeader.cpp
  dds/DCPS/transport/framework/TransportHeader.inl
  dds/DCPS/transport/framework/TransportImpl.cpp
  dds/DCPS/transport/framework/TransportImpl.inl
  dds/DCPS/transport/framework/TransportInst.cpp
  dds/DCPS/transport/framework/TransportInst.inl
  dds/DCPS/transport/framework/TransportQueueElement.cpp
  dds/DCPS/transport/framework/TransportQueueElement.inl
  dds/DCPS/transport/framework/TransportReassembly.cpp
  dds/DCPS/transport/framework/TransportReceiveListener.cpp
  dds/DCPS/transport/framework/TransportReceiveStrategy.cpp
  # dds/DCPS/transport/framework/TransportReceiveStrategy_T.cpp
  dds/DCPS/transport/framework/TransportReceiveStrategy_T.inl
  dds/DCPS/transport/framework/TransportRegistry.cpp
  dds/DCPS/transport/framework/TransportRegistry.inl
  dds/DCPS/transport/framework/TransportReplacedElement.cpp
  dds/DCPS/transport/framework/TransportReplacedElement.inl
  dds/DCPS/transport/framework/TransportRetainedElement.cpp
  dds/DCPS/transport/framework/TransportRetainedElement.inl
  dds/DCPS/transport/framework/TransportSendBuffer.cpp
  dds/DCPS/transport/framework/TransportSendBuffer.inl
  dds/DCPS/transport/framework/TransportSendControlElement.cpp
  dds/DCPS/transport/framework/TransportSendControlElement.inl
  dds/DCPS/transport/framework/TransportSendElement.cpp
  dds/DCPS/transport/framework/TransportSendElement.inl
  dds/DCPS/transport/framework/TransportSendListener.cpp
  dds/DCPS/transport/framework/TransportSendStrategy.cpp
  dds/DCPS/transport/framework/TransportSendStrategy.inl
  dds/DCPS/transport/framework/TransportStrategy.cpp
  dds/DCPS/transport/framework/TransportType.cpp
)
_opendds_library(OpenDDS_Dcps MSVC_BIGOBJ)
target_include_directories(OpenDDS_Dcps
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/dds"
)
target_link_libraries(OpenDDS_Dcps PUBLIC ${dcps_lib_deps})

set(opendds_target_sources_args
  AUTO_LINK FALSE
  USE_EXPORT "dds/DCPS/dcps_export.h" OpenDDS_Dcps_Export
  INCLUDE_BASE "${OPENDDS_SOURCE_ROOT}"
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

add_subdirectory(dds/DCPS/InfoRepoDiscovery)
add_subdirectory(dds/DCPS/RTPS)

add_subdirectory(dds/DCPS/transport/multicast)
add_subdirectory(dds/DCPS/transport/rtps_udp)
add_subdirectory(dds/DCPS/transport/shmem)
add_subdirectory(dds/DCPS/transport/tcp)
add_subdirectory(dds/DCPS/transport/udp)

add_subdirectory(dds/InfoRepo)

add_subdirectory(DevGuideExamples/DCPS/Messenger)
add_subdirectory(tests/cmake/ace_tao_only)
add_subdirectory(tests/cmake/Messenger)
# add_subdirectory(tests/cmake/idl_compiler_tests)
add_subdirectory(tests/cmake/include_subdir)
