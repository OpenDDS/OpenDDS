cmake_minimum_required(VERSION 3.3)

project(opendds_devguide_examples_dcps)

# Remove the MPC-generated headers so the CMake build will use the right ones.
# See Messenger/README.md for details.
file(GLOB messenger_headers "${CMAKE_CURRENT_LIST_DIR}/Messenger/*.h")
list(REMOVE_ITEM messenger_headers "${CMAKE_CURRENT_LIST_DIR}/Messenger/DataReaderListenerImpl.h")
list(LENGTH messenger_headers messenger_header_count)
if(messenger_header_count GREATER 0)
  file(REMOVE ${messenger_headers})
endif()
add_subdirectory("Messenger")
