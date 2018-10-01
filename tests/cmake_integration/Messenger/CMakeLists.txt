project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.3)

if (DEFINED ENV{DDS_ROOT})
  set(DDS_ROOT $ENV{DDS_ROOT})
  list(APPEND CMAKE_MODULE_PATH "${DDS_ROOT}/lib/cmake/Modules")
endif()

include(opendds_options)

find_package(OpenDDS)

# IDL Library
add_library(DDS_Messenger_Idl)
OPENDDS_TARGET_SOURCES(DDS_Messenger_Idl Messenger.idl)
target_link_libraries(OpenDDS::Dcps)

target_compile_definitions(DDS_Messenger_Idl
  PUBLIC
    OPENDDS_SECURITY)

# Publisher
add_executable(DDS_Messenger_Publisher)
target_sources(DDS_Messenger_Publisher
  PUBLIC
    Writer.cpp Writer.h publisher.cpp)

target_link_libraries(DDS_Messenger_Publisher
  DDS_Messenger_Idl OpenDDS::OpenDDS)

target_compile_definitions(DDS_Messenger_Publisher
  PUBLIC
    OPENDDS_SECURITY)

# Subscriber
add_executable(DDS_Messenger_Subscriber)
target_sources(DDS_Messenger_Subscriber
  PUBLIC
    DataReaderListener.cpp DataReaderListener.h subscriber.cpp)

target_link_libraries(DDS_Messenger_Subscriber
  DDS_Messenger_Idl OpenDDS::OpenDDS)

target_compile_definitions(DDS_Messenger_Subscriber
  PUBLIC
    OPENDDS_SECURITY)

# Stacksubscriber
add_executable(DDS_Messenger_Stacksubscriber)
target_sources(DDS_Messenger_Stacksubscriber
  PUBLIC
    DataReaderListener.cpp DataReaderListener.h stack_subscriber.cpp)

target_link_libraries(DDS_Messenger_Stacksubscriber
  DDS_Messenger_Idl OpenDDS::OpenDDS)

target_compile_definitions(DDS_Messenger_Stacksubscriber
  PUBLIC
    OPENDDS_SECURITY)
