cmake_minimum_required(VERSION 3.23...3.27)
project(opendds_info_repo CXX)

set(lib_dep_libs
  OpenDDS::Dcps
  OpenDDS::InfoRepoDiscovery
  OpenDDS::Tcp
  TAO::IORTable
  TAO::ImR_Client
  TAO::Svc_Utils
)
set(exe_dep_libs
  TAO::PI
  TAO::BiDirGIOP
)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${lib_dep_libs} ${exe_dep_libs} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_InfoRepoLib
  DCPSInfo_i.cpp
  DcpsInfo_pch.cpp
  DCPS_IR_Domain.cpp
  DCPS_IR_Participant.cpp
  DCPS_IR_Publication.cpp
  DCPS_IR_Subscription.cpp
  DCPS_IR_Topic.cpp
  DCPS_IR_Topic_Description.cpp
  DomainParticipantListener_i.cpp
  FederationId.cpp
  PersistenceUpdater.cpp
  UpdateManager.cpp
)
_opendds_library(OpenDDS_InfoRepoLib)
target_link_libraries(OpenDDS_InfoRepoLib PUBLIC ${lib_dep_libs})

add_library(OpenDDS_Federator
  FederatorConfig.cpp
  FederatorManagerImpl.cpp
  FederatorManagerImpl_updates.cpp
  InfoRepoMulticastResponder.cpp
)
target_sources(OpenDDS_Federator
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    federator_export.h
    UpdateListener_T.h
    UpdateReceiver_T.h
    UpdateProcessor_T.h
)
_opendds_library(OpenDDS_Federator MSVC_BIGOBJ)
target_link_libraries(OpenDDS_Federator PUBLIC OpenDDS::InfoRepoLib)
target_compile_definitions(OpenDDS_Federator PRIVATE OPENDDS_FEDERATOR_BUILD_DLL)
opendds_target_sources(OpenDDS_Federator
  PUBLIC
    Federator.idl
  INCLUDE_BASE "${OPENDDS_SOURCE_DIR}"
  GENERATE_SERVER_SKELETONS TRUE
  USE_EXPORT "dds/InfoRepo/federator_export.h" OpenDDS_Federator_Export
)
opendds_install_interface_files(OpenDDS_Federator INCLUDE_BASE "${OPENDDS_SOURCE_DIR}")

add_library(OpenDDS_InfoRepoServ DCPSInfoRepoServ.cpp)
target_sources(OpenDDS_InfoRepoServ
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    DCPSInfoRepoServ.h
)
_opendds_library(OpenDDS_InfoRepoServ)
target_link_libraries(OpenDDS_InfoRepoServ PUBLIC OpenDDS::Federator)

add_executable(DCPSInfoRepo DCPSInfoRepo.cpp)
_opendds_executable(DCPSInfoRepo)
target_link_libraries(DCPSInfoRepo PRIVATE OpenDDS::InfoRepoServ ${exe_dep_libs})
