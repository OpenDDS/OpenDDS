cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_devguide_messenger CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

set(BINARY_TEST_FACE_DIR ${CMAKE_CURRENT_BINARY_DIR}/idlTestFace)
file(MAKE_DIRECTORY ${BINARY_TEST_FACE_DIR})

execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/init.sh ${CMAKE_CURRENT_SOURCE_DIR}/FaceMessage.idl ${BINARY_TEST_FACE_DIR}/FaceMessageTypeSupport.idl ${BINARY_TEST_FACE_DIR}
)

# Make sure the MPC-generated headers are gone so the CMake build will use the
# right ones. This is not needed in a real project.
file(GLOB headers "*.h")
list(LENGTH headers header_count)
if (header_count GREATER 0)
    file(REMOVE ${headers})
endif ()

# IDL TypeSupport Library
add_library(messenger_idl)
opendds_target_sources(messenger_idl PUBLIC "FaceMessage.idl")
target_link_libraries(messenger_idl PUBLIC OpenDDS::FACE)

set(opendds_libs
        OpenDDS::Rtps OpenDDS::Rtps_Udp
        messenger_idl
)

# Publisher
add_executable(publisher.out
        Publisher.cpp
        build/idlTestFace/FaceMessage_TS.cpp
)
target_link_libraries(publisher.out ${opendds_libs})

# Subscriber
add_executable(subscriber.out
        Subscriber.cpp
        build/idlTestFace/FaceMessage_TS.cpp
)
target_link_libraries(subscriber.out ${opendds_libs})
