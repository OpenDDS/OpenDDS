# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# To build this project follow these steps:
#   1) mkdir build
#   2) cd build
#   3) cmake -DCMAKE_PREFIX_PATH=$DDS_ROOT ..
#   4) cmake --build .
#

project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS)

set(src "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets publisher subscriber stack_subscriber)

# Publisher
add_executable(publisher
    ${src}/publisher.cpp
)
OPENDDS_TARGET_SOURCES(publisher
  PUBLIC
    ${src}/Writer.cpp
    ${src}/Writer.h
    ${src}/Messenger.idl
)

# Subscriber
add_executable(subscriber
    ${src}/subscriber.cpp
)
OPENDDS_TARGET_SOURCES(subscriber
  PUBLIC
    ${src}/DataReaderListener.cpp
    ${src}/DataReaderListener.h
    ${src}/Messenger.idl
)

# Stacksubscriber
add_executable(stack_subscriber
    ${src}/stack_subscriber.cpp
)
OPENDDS_TARGET_SOURCES(stack_subscriber
  PUBLIC
    ${src}/DataReaderListener.cpp
    ${src}/DataReaderListener.h
    ${src}/Messenger.idl
)

foreach(t ${all_targets})
  target_link_libraries(${t} OpenDDS::OpenDDS)
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
