cmake_minimum_required(VERSION 3.14...4.0)
project(opendds_qos_xml_xsc_handler CXX)

set(deps OpenDDS::Dcps ACE::XML_Utils)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps})
include(opendds_build_helpers)

add_library(OpenDDS_QOS_XML_XSC_Handler
  DataReaderQos_Handler.cpp
  DataWriterQos_Handler.cpp
  ParticipantQos_Handler.cpp
  PublisherQos_Handler.cpp
  QOS_Common.cpp
  QOS_XML_Loader.cpp
  SubscriberQos_Handler.cpp
  TopicQos_Handler.cpp
  XML_File_Intf.cpp
  XML_Intf.cpp
  XML_String_Intf.cpp
  dds_qos.cpp
)
_opendds_headers(OpenDDS_QOS_XML_XSC_Handler
  PUBLIC
    DataReaderQos_Handler.h
    DataWriterQos_Handler.h
    OpenDDS_XML_QOS_Handler_Export.h
    ParticipantQos_Handler.h
    PublisherQos_Handler.h
    QOS_Common.h
    QOS_DataReader_T.cpp
    QOS_DataReader_T.h
    QOS_DataWriter_T.cpp
    QOS_DataWriter_T.h
    QOS_DwDrTp_Base_T.cpp
    QOS_DwDrTp_Base_T.h
    QOS_DwTp_Base_T.cpp
    QOS_DwTp_Base_T.h
    QOS_PubSub_T.cpp
    QOS_PubSub_T.h
    QOS_Topic_T.cpp
    QOS_Topic_T.h
    QOS_XML_Loader.h
    SubscriberQos_Handler.h
    TopicQos_Handler.h
    XML_File_Intf.h
    XML_Intf.h
    XML_String_Intf.h
    dds_qos.hpp
)
_opendds_library(OpenDDS_QOS_XML_XSC_Handler
  EXPORT_MACRO_PREFIX OpenDDS_XML_QOS_Handler
)
target_link_libraries(OpenDDS_QOS_XML_XSC_Handler PUBLIC ${deps})
