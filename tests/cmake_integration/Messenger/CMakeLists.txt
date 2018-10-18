# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.3)

if (DEFINED ENV{DDS_ROOT})
  set(DDS_ROOT $ENV{DDS_ROOT})
  list(APPEND CMAKE_MODULE_PATH "${DDS_ROOT}/lib/cmake/Modules")
endif()

find_package(OpenDDS)

# IDL Library
add_library(messenger)
OPENDDS_TARGET_SOURCES(messenger Messenger.idl)
target_link_libraries(OpenDDS::Dcps)

# Publisher
add_executable(publisher)
target_sources(publisher
  PUBLIC
    Writer.cpp Writer.h publisher.cpp)

target_link_libraries(publisher
  messenger OpenDDS::OpenDDS)

# Subscriber
add_executable(subscriber)
target_sources(subscriber
  PUBLIC
    DataReaderListener.cpp DataReaderListener.h subscriber.cpp)

target_link_libraries(subscriber
  messenger OpenDDS::OpenDDS)

# Stacksubscriber
add_executable(stack_subscriber)
target_sources(stack_subscriber
  PUBLIC
    DataReaderListener.cpp DataReaderListener.h stack_subscriber.cpp)

target_link_libraries(stack_subscriber
  messenger OpenDDS::OpenDDS)

# Copy configs/scripts into build-output directory
set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR})
set(_dst_dir ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB _ini_src "${_src_dir}/*.ini")
file(GLOB _pl_src "${_src_dir}/*.pl")
add_custom_target(Copy_ini_and_scripts
  ALL
  COMMAND_EXPAND_LISTS
  VERBATIM
  COMMENT "Copying configs/scripts into build-output directory"
  COMMAND ${CMAKE_COMMAND} -E
    copy ${_ini_src} ${_pl_src} ${_dst_dir}/$<CONFIG>)

set(_post_build_deps
  messenger
  publisher
  subscriber
  stack_subscriber)

add_dependencies(Copy_ini_and_scripts ${_post_build_deps})
