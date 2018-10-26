# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

list(APPEND CMAKE_MODULE_PATH "$ENV{DDS_ROOT}/share/cmake/Modules")
find_package(OpenDDS)

set(src "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets publisher subscriber stack_subscriber messenger)

# Messenger library
add_library(messenger)
OPENDDS_TARGET_SOURCES(messenger
  PUBLIC
    ${src}/Messenger.idl)

# Publisher
add_executable(publisher
    ${src}/publisher.cpp
)
OPENDDS_TARGET_SOURCES(publisher
  PUBLIC
    ${src}/Writer.cpp
    ${src}/Writer.h
)

# Subscriber
add_executable(subscriber
    ${src}/subscriber.cpp
)
OPENDDS_TARGET_SOURCES(subscriber
  PUBLIC
    ${src}/DataReaderListener.cpp
    ${src}/DataReaderListener.h
)

# Stacksubscriber
add_executable(stack_subscriber
    ${src}/stack_subscriber.cpp
)
OPENDDS_TARGET_SOURCES(stack_subscriber
  PUBLIC
    ${src}/DataReaderListener.cpp
    ${src}/DataReaderListener.h
)

foreach(t ${all_targets})
  target_link_libraries(${t} OpenDDS::OpenDDS)

  if (NOT "${t}" STREQUAL "messenger")
    target_link_libraries(${t} messenger)
  endif()
endforeach()

# Copy configs/scripts into build-output directory
file(GLOB inis "${src}/Configs/*.ini")
file(GLOB pls "${src}/Scripts/*.pl")
add_custom_target(Copy_ini_and_scripts
  ALL
  COMMAND_EXPAND_LISTS
  VERBATIM
  COMMENT "Copying configs/scripts into build-output directory"
  COMMAND ${CMAKE_COMMAND} -E copy ${inis} ${pls} ${dst}/$<CONFIG>
)

add_dependencies(Copy_ini_and_scripts ${all_targets})
