# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#

project(OpenDDS_Messenger1_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})

if (NOT OPENDDS_OWNERSHIP_PROFILE)
  message(FATAL_ERROR "These tests require OpenDDS to be built with Ownership Profile enabled")
endif()

set(target_prefix "opendds_cmake_messenger_1_")
set(publisher "${target_prefix}publisher")
set(subscriber "${target_prefix}subscriber")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets ${publisher} ${subscriber})

foreach(file
  Messenger.idl subscriber.cpp publisher.cpp Writer.cpp Writer.h Args.h
  DataReaderListener.cpp DataReaderListener.h)
  configure_file(${src}/${file} ${dst}/${file} COPYONLY)
endforeach()

# Publisher
add_executable(${publisher}
    ${dst}/publisher.cpp
)
set_target_properties(${publisher}
  PROPERTIES OUTPUT_NAME publisher
)
OPENDDS_TARGET_SOURCES(${publisher}
    ${dst}/Writer.cpp
    ${dst}/Writer.h
    ${dst}/Messenger.idl
)

# Subscriber
add_executable(${subscriber}
    ${dst}/subscriber.cpp
)
set_target_properties(${subscriber}
  PROPERTIES OUTPUT_NAME subscriber
)
OPENDDS_TARGET_SOURCES(${subscriber}
    ${dst}/DataReaderListener.cpp
    ${dst}/DataReaderListener.h
    ${dst}/Messenger.idl
)

foreach(t ${all_targets})
  # Using explicit scope here is a regression test for
  # https://github.com/objectcomputing/OpenDDS/issues/1336
  # See https://cmake.org/cmake/help/latest/policy/CMP0023.html
  target_link_libraries(${t} PUBLIC OpenDDS::OpenDDS)
endforeach()

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
