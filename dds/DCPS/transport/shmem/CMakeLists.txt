cmake_minimum_required(VERSION 3.14...4.0)
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
)
_opendds_headers(OpenDDS_Shmem
  PUBLIC
    Shmem.h
    ShmemAllocator.h
    ShmemDataLink.h
    ShmemDataLink.inl
    ShmemDataLink_rch.h
    ShmemInst.h
    ShmemInst_rch.h
    ShmemLoader.h
    ShmemReceiveStrategy.h
    ShmemReceiveStrategy_rch.h
    ShmemSendStrategy.h
    ShmemSendStrategy_rch.h
    ShmemTransport.h
    ShmemTransport_rch.h
    Shmem_Export.h
)
_opendds_library(OpenDDS_Shmem)
target_link_libraries(OpenDDS_Shmem PUBLIC ${deps})
