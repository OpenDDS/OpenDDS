project(Messenger_cmake CXX)
cmake_minimum_required(VERSION 3.3)

if (DEFINED ENV{DDS_ROOT})
  set(DDS_ROOT $ENV{DDS_ROOT})
  list(APPEND CMAKE_MODULE_PATH "${DDS_ROOT}/lib/cmake/Modules")
endif()

include(opendds_options)

find_package(OpenDDS)

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
  OpenDDS::OpenDDS DDS_Messenger_Idl
  Writer.cpp Writer.h publisher.cpp)

target_compile_definitions(DDS_Messenger_Publisher
  PUBLIC
    OPENDDS_SECURITY)

OPENDDS_ADD_EXECUTABLE(DDS_Messenger_Subscriber
  OpenDDS::OpenDDS DDS_Messenger_Idl
  DataReaderListener.cpp DataReaderListener.h subscriber.cpp)

target_compile_definitions(DDS_Messenger_Subscriber
  PUBLIC
    OPENDDS_SECURITY)

OPENDDS_ADD_EXECUTABLE(DDS_Messenger_Stacksubscriber
  OpenDDS::OpenDDS DDS_Messenger_Idl
  DataReaderListener.cpp DataReaderListener.h stack_subscriber.cpp)

target_compile_definitions(DDS_Messenger_Stacksubscriber
  PUBLIC
    OPENDDS_SECURITY)

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
