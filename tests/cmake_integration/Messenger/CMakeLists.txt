project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.3)

if (DEFINED ENV{DDS_ROOT})
  set(DDS_ROOT $ENV{DDS_ROOT})
  list(APPEND CMAKE_MODULE_PATH "${DDS_ROOT}/lib/cmake/Modules")
endif()

include(opendds_options)

find_package(OpenDDS 3 REQUIRED)

# ace_requires("TARGET tao_nsadd")

ace_add_lib(DDS_Messenger_Idl
  OUTPUT_NAME DDS_Messenger_Idl
  PUBLIC_COMPILE_DEFINITIONS MESSENGER_BUILD_DLL
  PUBLIC_LINK_LIBRARIES OpenDDS::Dcps
  INCLUDE_DIRECTORIES "${OPENDDS_INCLUDE_DIRS}"
)

dds_idl_sources(
  TARGETS DDS_Messenger_Idl
  DDS_IDL_FLAGS -Wb,export_macro=Messenger_Export
  TAO_IDL_FLAGS -Wb,stub_export_include=Messenger_export.h
                -Wb,stub_export_macro=Messenger_Export
                -SS
  IDL_FILES Messenger.idl
)

ace_add_exe(DDS_Messenger_Publisher
  OUTPUT_NAME publisher
  LINK_LIBRARIES ACE::ACE
                 TAO::TAO
                 OpenDDS::Tcp
                 OpenDDS::Udp
                 OpenDDS::Multicast
                 OpenDDS::Rtps_Udp
                 OpenDDS::Shmem
                 OpenDDS::InfoRepoDiscovery
                 OpenDDS::Rtps
                 OpenDDS::Dcps
                 DDS_Messenger_Idl
)

ace_target_cxx_sources(DDS_Messenger_Publisher
  SOURCE_FILES Writer.cpp
               publisher.cpp
  HEADER_FILES Writer.h
)

ace_add_exe(DDS_Messenger_Subscriber
  OUTPUT_NAME subscriber
  LINK_LIBRARIES ACE::ACE
                 TAO::TAO
                 OpenDDS::Tcp
                 OpenDDS::Udp
                 OpenDDS::Multicast
                 OpenDDS::Rtps_Udp
                 OpenDDS::Shmem
                 OpenDDS::InfoRepoDiscovery
                 OpenDDS::Rtps
                 OpenDDS::Dcps
                 DDS_Messenger_Idl
)
ace_target_cxx_sources(DDS_Messenger_Subscriber
  SOURCE_FILES DataReaderListener.cpp
               subscriber.cpp
  HEADER_FILES DataReaderListener.h
)


ace_add_exe(DDS_Messenger_Stacksubscriber
  OUTPUT_NAME stack_subscriber
  LINK_LIBRARIES ACE::ACE
                 TAO::TAO
                 OpenDDS::Tcp
                 OpenDDS::Udp
                 OpenDDS::Multicast
                 OpenDDS::Rtps_Udp
                 OpenDDS::Shmem
                 OpenDDS::InfoRepoDiscovery
                 OpenDDS::Rtps
                 OpenDDS::Dcps
                 DDS_Messenger_Idl
)
ace_target_cxx_sources(DDS_Messenger_Stacksubscriber
  SOURCE_FILES DataReaderListener.cpp
               stack_subscriber.cpp
  HEADER_FILES DataReaderListener.h
)

# include(${OpenDDS_INCLUDE_DIR}/cmake/dds_add_test.cmake)

# set(replace_nsadd "\"\$ENV{ACE_ROOT}/bin/tao_nsadd\"" "PerlDDS::tao_nsadd()")
# set(replace_nsadd_check_condition  "! -s \$nsadd.\$exec_extn" "0")

# if (ACE_MULTI_CONFIGURATION_GENERATOR AND TAO_INSTALL_DIR)
  # set(shouldnt_ignore_exesubdir "\$NSADD->IgnoreExeSubDir(1)\;" " ")
# endif()

# dds_configure_test_files(replace_nsadd replace_nsadd_check_condition shouldnt_ignore_exesubdir)

file(GLOB files *.ini)
file(COPY ${files}
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
