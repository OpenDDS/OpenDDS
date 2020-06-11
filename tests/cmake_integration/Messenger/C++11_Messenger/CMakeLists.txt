project(OpenDDS_C++11_Messenger CXX)
cmake_minimum_required(VERSION 3.8.2)

find_package(OpenDDS REQUIRED)

set(CMAKE_CXX_COMPILER ${OPENDDS_COMPILER})
set(target_prefix "opendds_cmake_cpp11_messenger_")
set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../../DCPS/C++11/Messenger")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(opendds_libs
  OpenDDS::Dcps
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp
)

# IDL Library
set(idl "${target_prefix}idl")
add_library(${idl})
OPENDDS_TARGET_SOURCES(${idl} "${src}/Idl/Messenger.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
set_target_properties(${idl} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Idl"
)

# Assert the mapping used was C++11
get_property(mappings TARGET ${idl} PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
if(NOT ("C++11" IN_LIST mappings))
  message(FATAL_ERROR "${idl}: C++11 NOT in mapping list: ${mappings}")
endif()

# Publisher
set(publisher "${target_prefix}publisher")
add_executable(${publisher}
  "${src}/Publisher/publisher.cpp"
)
set_target_properties(${publisher} PROPERTIES
  OUTPUT_NAME "publisher"
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Publisher"
)
target_link_libraries(${publisher} ${opendds_libs} ${idl})

# Subscriber
set(subscriber "${target_prefix}subscriber")
add_executable(${subscriber}
  "${src}/Subscriber/subscriber.cpp"
)
set_target_properties(${subscriber} PROPERTIES
  OUTPUT_NAME "subscriber"
  RUNTIME_OUTPUT_DIRECTORY "${dst}/Subscriber"
)
target_link_libraries(${subscriber} ${opendds_libs} ${idl})

# run_test.pl
configure_file("${src}/run_test.pl" "${dst}/run_test.pl" COPYONLY)
