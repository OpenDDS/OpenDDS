cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_ace_tao_only CXX)
enable_testing()

find_package(OpenDDS REQUIRED COMPONENTS NO_DEFAULTS TAO::tao_idl TAO::PortableServer)
include(opendds_testing)

_opendds_save_cache(OPENDDS_AUTO_LINK_DCPS BOOL TRUE)

set(target_prefix "${PROJECT_NAME}_")
set(idl "${target_prefix}idl")
set(server "${target_prefix}server")
set(client "${target_prefix}client")

set(path "DevGuideExamples/GettingStarted")
set(src "${OPENDDS_TAO}/${path}")
if(NOT EXISTS "${src}")
  set(src "$ENV{TAO_SOURCE_ROOT}/${path}")
  if(NOT EXISTS "${src}")
    message(FATAL_ERROR "This test requires $TAO_SOURCE_ROOT to be set!")
  endif()
endif()
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets ${server} ${client})

set(idl_file Messenger.idl)
set(server_files
  Messenger_i.cpp
  MessengerServer.cpp
)
set(client_files
  MessengerClient.cpp
)
set(other_files
  Messenger_i.h
  started_pch.cpp
  started_pch.h
  run_test.pl
)
foreach(file ${idl_file} ${server_files} ${client_files} ${other_files})
  configure_file("${src}/${file}" "${dst}/${file}" COPYONLY)
endforeach()

add_library(${idl})
opendds_target_sources(${idl}
  PUBLIC "${dst}/${idl_file}"
  SKIP_OPENDDS_IDL
  GENERATE_SERVER_SKELETONS TRUE
)
target_link_libraries(${idl} PUBLIC TAO::PortableServer)

add_executable(${server} ${server_files})
set_target_properties(${server} PROPERTIES OUTPUT_NAME MessengerServer)
target_link_libraries(${server} ${idl})

add_executable(${client} ${client_files})
set_target_properties(${client} PROPERTIES OUTPUT_NAME MessengerClient)
target_link_libraries(${client} ${idl})

opendds_add_test(ASSUME_RUN_TEST)

_opendds_restore_cache()
