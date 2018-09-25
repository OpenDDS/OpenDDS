project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.3)

if (DEFINED ENV{DDS_ROOT})
  set(DDS_ROOT $ENV{DDS_ROOT})
  list(APPEND CMAKE_MODULE_PATH "${DDS_ROOT}/lib/cmake/Modules")
endif()

include(opendds_options)

find_package(OpenDDS 3 REQUIRED)

OPENDDS_ADD_LIBRARY(DDS_Messenger_Idl
  OpenDDS::Dcps
  Messenger.idl
  TAO_IDL_OPTIONS
    -Wb,stub_export_include=Messenger_export.h
    -Wb,stub_export_macro=Messenger_Export
    -SS
  OPENDDS_IDL_OPTIONS
    -Wb,export_macro=Messenger_Export)

target_compile_definitions(DDS_Messenger_Idl
  PUBLIC
    OPENDDS_SECURITY
    MESSENGER_BUILD_DLL)

OPENDDS_ADD_EXECUTABLE(DDS_Messenger_Publisher
  ACE::ACE
  TAO::TAO
  OpenDDS::Tcp
  OpenDDS::Udp
  OpenDDS::Multicast
  OpenDDS::Rtps_Udp
  OpenDDS::Shmem
  OpenDDS::InfoRepoDiscovery
  OpenDDS::Rtps
  OpenDDS::Dcps
  OpenDDS::Security
  DDS_Messenger_Idl
  Writer.cpp
  publisher.cpp
  Writer.h)

target_compile_definitions(DDS_Messenger_Publisher
  PUBLIC
    OPENDDS_SECURITY)

# ace_add_exe(DDS_Messenger_Subscriber
  # OUTPUT_NAME subscriber
  # LINK_LIBRARIES ACE::ACE
                 # TAO::TAO
                 # OpenDDS::Tcp
                 # OpenDDS::Udp
                 # OpenDDS::Multicast
                 # OpenDDS::Rtps_Udp
                 # OpenDDS::Shmem
                 # OpenDDS::InfoRepoDiscovery
                 # OpenDDS::Rtps
                 # OpenDDS::Dcps
                 # OpenDDS::Security
                 # DDS_Messenger_Idl
  # COMPILE_DEFINITIONS OPENDDS_SECURITY
# )
# ace_target_cxx_sources(DDS_Messenger_Subscriber
  # SOURCE_FILES DataReaderListener.cpp
               # subscriber.cpp
  # HEADER_FILES DataReaderListener.h
# )


# ace_add_exe(DDS_Messenger_Stacksubscriber
  # OUTPUT_NAME stack_subscriber
  # LINK_LIBRARIES ACE::ACE
                 # TAO::TAO
                 # OpenDDS::Tcp
                 # OpenDDS::Udp
                 # OpenDDS::Multicast
                 # OpenDDS::Rtps_Udp
                 # OpenDDS::Shmem
                 # OpenDDS::InfoRepoDiscovery
                 # OpenDDS::Rtps
                 # OpenDDS::Dcps
                 # OpenDDS::Security
                 # DDS_Messenger_Idl
  # COMPILE_DEFINITIONS OPENDDS_SECURITY
# )
# ace_target_cxx_sources(DDS_Messenger_Stacksubscriber
  # SOURCE_FILES DataReaderListener.cpp
               # stack_subscriber.cpp
  # HEADER_FILES DataReaderListener.h
# )

# include(${OpenDDS_INCLUDE_DIR}/cmake/dds_add_test.cmake)

# set(replace_nsadd "\"\$ENV{ACE_ROOT}/bin/tao_nsadd\"" "PerlDDS::tao_nsadd()")
# set(replace_nsadd_check_condition  "! -s \$nsadd.\$exec_extn" "0")

# if (ACE_MULTI_CONFIGURATION_GENERATOR AND TAO_INSTALL_DIR)
  # set(shouldnt_ignore_exesubdir "\$NSADD->IgnoreExeSubDir(1)\;" " ")
# endif()

# dds_configure_test_files(replace_nsadd replace_nsadd_check_condition shouldnt_ignore_exesubdir)

#file(GLOB files *.ini)
#file(COPY ${files}
#     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
