cmake_minimum_required(VERSION 3.23...3.27)
project(opendds_idl CXX)

include(GNUInstallDirs)

set(deps ACE::ACE TAO::IDL_FE)
find_package(OpenDDS REQUIRED COMPONENTS NO_DEFAULTS ${deps})
include(opendds_build_helpers)

add_executable(opendds_idl
  annotations.cpp
  be_global.cpp
  be_init.cpp
  be_produce.cpp
  be_util.cpp
  dds_generator.cpp
  dds_visitor.cpp
  drv_args.cpp
  drv_preproc.cpp
  dynamic_data_adapter_generator.cpp
  field_info.cpp
  itl_generator.cpp
  keys_generator.cpp
  langmap_generator.cpp
  marshal_generator.cpp
  metaclass_generator.cpp
  opendds_idl.cpp
  topic_keys.cpp
  ts_generator.cpp
  typeobject_generator.cpp
  value_reader_generator.cpp
  value_writer_generator.cpp
)
_opendds_executable(opendds_idl)
target_link_libraries(opendds_idl PRIVATE ${deps} OpenDDS_Util)
set_target_properties(opendds_idl PROPERTIES
  # opendds_idl doesn't play nice with bundling the cpp files together
  UNITY_BUILD FALSE
)

install(FILES IDLTemplate.txt DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/dds/dds/idl")
