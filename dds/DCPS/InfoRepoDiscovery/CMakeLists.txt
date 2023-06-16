project(opendds_info_repo_discovery CXX)

set(lib_deps OpenDDS::Dcps OpenDDS::Tcp TAO::TAO TAO::PI)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${lib_deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_InfoRepoDiscovery
  DataReaderRemoteImpl.cpp
  DataWriterRemoteImpl.cpp
  FailoverListener.cpp
  InfoRepoDiscovery.cpp
)
_opendds_library(OpenDDS_InfoRepoDiscovery)
target_link_libraries(OpenDDS_InfoRepoDiscovery PUBLIC ${lib_deps})

opendds_target_sources(OpenDDS_InfoRepoDiscovery
  PUBLIC
    Info.idl
    DataReaderRemote.idl
    DataWriterRemote.idl
  INCLUDE_BASE "${OPENDDS_SOURCE_ROOT}"
  SKIP_OPENDDS_IDL
  GENERATE_SERVER_SKELETONS TRUE
  USE_EXPORT "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery_Export.h" OpenDDS_InfoRepoDiscovery_Export
)
