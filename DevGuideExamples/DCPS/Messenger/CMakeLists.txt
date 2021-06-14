project(opendds_devguide_messenger CXX)
cmake_minimum_required(VERSION 3.3)
enable_testing()

find_package(OpenDDS REQUIRED)

# Make sure the MPC-generated headers are gone so the CMake build will use the
# right ones. This is not needed in a real project.
file(GLOB headers "*.h")
list(REMOVE_ITEM headers "DataReaderListenerImpl.h")
list(LENGTH _headers header_count)
if(header_count GREATER 0)
  file(REMOVE ${headers})
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

# Testing
configure_file(run_test.pl . COPYONLY)
configure_file(rtps.ini . COPYONLY)
add_test(NAME opendds_devguide_messenger_test_inforepo
  COMMAND perl run_test.pl $<$<BOOL:$<CONFIG>>:-ExeSubDir> $<CONFIG>
)
add_test(NAME opendds_devguide_messenger_test_rtps
  COMMAND perl run_test.pl $<$<BOOL:$<CONFIG>>:-ExeSubDir> $<CONFIG> --rtps
)
