cmake_minimum_required(VERSION 3.23...4.0)
project(opendds_xtypes_xml CXX)

set(deps
  XercesC::XercesC
)
find_package(OpenDDS REQUIRED NO_DEFAULTS OpenDDS::Dcps safety_profile=FALSE)
find_package(XercesC REQUIRED)
include(opendds_build_helpers)

add_library(OpenDDS_XTypes_Xml
  XmlTypeProvider.cpp
)
target_sources(OpenDDS_XTypes_Xml
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    XmlTypeProvider.h
    XmlTypeProvider_export.h
)
_opendds_library(OpenDDS_XTypes_Xml)
target_include_directories(OpenDDS_XTypes_Xml
  PRIVATE
    "${OPENDDS_SOURCE_DIR}/dds"
)
target_compile_definitions(OpenDDS_XTypes_Xml
  PUBLIC
    OPENDDS_XERCES3
)
target_link_libraries(OpenDDS_XTypes_Xml PUBLIC ${deps})
target_link_libraries(OpenDDS_XTypes_Xml PUBLIC OpenDDS::Dcps)
