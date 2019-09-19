project(OpenDDS_DevGuide_Messenger CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS)

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})

if (NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "Messenger requires OpenDDS to be built with Ownership Profile enabled")
endif()

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp # For run_test.pl
  OpenDDS::Rtps OpenDDS::Rtps_Udp # For run_test.pl --rtps
)

# Publisher
add_executable(publisher
  Publisher.cpp
)
OPENDDS_TARGET_SOURCES(publisher Messenger.idl)
target_link_libraries(publisher ${opendds_libs})

# Subscriber
add_executable(subscriber
  Subscriber.cpp
  DataReaderListenerImpl.cpp
)
OPENDDS_TARGET_SOURCES(subscriber Messenger.idl)
target_link_libraries(subscriber ${opendds_libs})
