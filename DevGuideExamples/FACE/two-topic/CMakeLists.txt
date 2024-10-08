cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_face_stock CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

# Make sure the MPC-generated headers are gone so the CMake build will use the
# right ones. This is not needed in a real project.
file(GLOB headers "*.h")
list(LENGTH headers header_count)
if(header_count GREATER 0)
  file(REMOVE ${headers})
endif()

# IDL TypeSupport Library
set(ecu_lib "${target_prefix}lib") 
add_library(${ecu_lib})

target_link_libraries(${ecu_lib} PUBLIC OpenDDS::FACE)
opendds_target_sources(${ecu_lib} PUBLIC "StockQuoter.idl")

set(opendds_libs
  OpenDDS::Rtps OpenDDS::Rtps_Udp 
  OpenDDS::FACE
  ${ecu_lib}
)

# Publisher
add_executable(publisher
  Publisher.cpp
  StockQuoter_TS.cpp
)
target_link_libraries(publisher ${opendds_libs})

# Subscriber
add_executable(subscriber
  Subscriber.cpp
  StockQuoter_TS.cpp
)
target_link_libraries(subscriber ${opendds_libs})
