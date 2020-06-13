# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file is setup the way it is for testing purposes. For a cleaner example
# see DevGuideExamples/DCPS/Messenger/CMakeLists.txt

project(OpenDDS_Messenger2_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})

if (NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "These tests require OpenDDS to be built with Ownership Profile enabled")
endif()

set(target_prefix "opendds_cmake_messenger_2_")
set(publisher "${target_prefix}publisher")
set(subscriber "${target_prefix}subscriber")
set(messenger "${target_prefix}messenger")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets ${publisher} ${subscriber} ${messenger})

foreach(file
  Messenger.idl subscriber.cpp publisher.cpp Writer.cpp Writer.h Args.h
  DataReaderListener.cpp DataReaderListener.h)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

# Messenger library
add_library(${messenger})
set_target_properties(${messenger}
  PROPERTIES OUTPUT_NAME messenger
)
OPENDDS_TARGET_SOURCES(${messenger} "${dst}/Messenger.idl")
target_link_libraries(${messenger} OpenDDS::Dcps)

# Publisher
add_executable(${publisher}
  "${dst}/publisher.cpp"
  "${dst}/Writer.h"
  "${dst}/Writer.cpp"
)
set_target_properties(${publisher}
  PROPERTIES OUTPUT_NAME publisher
)
target_link_libraries(${publisher} ${messenger} OpenDDS::OpenDDS)

# Subscriber
add_executable(${subscriber}
  "${dst}/subscriber.cpp"
  "${dst}/DataReaderListener.h"
  "${dst}/DataReaderListener.cpp"
)
set_target_properties(${subscriber}
  PROPERTIES OUTPUT_NAME subscriber
)
target_link_libraries(${subscriber} ${messenger} OpenDDS::OpenDDS)

# Copy configs/scripts into build-output directory
file(GLOB ini "${src}/*.ini")
file(GLOB pl "${src}/*.pl")
file(GLOB xml "${src}/*.xml")
file(GLOB p7s "${src}/*.p7s")

add_custom_target("${target_prefix}Copy_ini_and_scripts"
  ALL
  COMMAND_EXPAND_LISTS
  VERBATIM
  COMMENT "Copying configs/scripts into build-output directory"
  COMMAND ${CMAKE_COMMAND} -E copy ${ini} ${pl} ${xml} ${p7s} ${dst}/$<CONFIG>
)
add_dependencies("${target_prefix}Copy_ini_and_scripts" ${all_targets})
