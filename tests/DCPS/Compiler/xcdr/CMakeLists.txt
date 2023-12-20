cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_compiler_xcdr CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(${PROJECT_NAME} "xcdr.cpp")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME xcdr)
opendds_target_sources(${PROJECT_NAME}
  PUBLIC
    xcdrbasetypes.idl
    appendable_mixed.idl
    mutable_types.idl
    mutable_types2.idl
    keyonly.idl
  OPENDDS_IDL_OPTIONS --no-default-nested -Gxtypes-complete
)
_opendds_msvc_bigobj(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Dcps OpenDDS_GoogleTest)

opendds_add_test(NAME static)
if(OPENDDS_CONTENT_SUBSCRIPTION)
  opendds_add_test(NAME dynamic ARGS --dynamic)
endif()
