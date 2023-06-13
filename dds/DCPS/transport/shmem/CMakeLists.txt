project(opendds_shmem CXX)

set(deps OpenDDS::Dcps)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
include(opendds_build_helpers)

add_library(OpenDDS_Shmem
  Shmem.cpp
  ShmemDataLink.cpp
  ShmemInst.cpp
  ShmemLoader.cpp
  ShmemReceiveStrategy.cpp
  ShmemSendStrategy.cpp
  ShmemTransport.cpp

  ShmemDataLink.inl
)
_opendds_library(OpenDDS_Shmem)
target_link_libraries(OpenDDS_Shmem PUBLIC ${deps})
