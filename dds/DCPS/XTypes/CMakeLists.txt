cmake_minimum_required(VERSION 3.23...4.0)
project(opendds_xtypes_xml CXX)

set(deps
  OpenDDS::Dcps
  XercesC::XercesC
)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} safety_profile=FALSE)
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
target_compile_definitions(OpenDDS_XTypes_Xml
  PUBLIC
    OPENDDS_XERCES3
)
target_link_libraries(OpenDDS_XTypes_Xml PUBLIC ${deps})

opendds_install_interface_files(OpenDDS_XTypes_Xml INCLUDE_BASE "${OPENDDS_SOURCE_DIR}")
