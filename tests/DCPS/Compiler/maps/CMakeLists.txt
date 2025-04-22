cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_compiler_maps CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(${PROJECT_NAME}  "main.cpp")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME maps)
opendds_target_sources(${PROJECT_NAME} PUBLIC "test.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${PROJECT_NAME} OpenDDS::Dcps gtest)

configure_file(run_test.pl . COPYONLY)
opendds_add_test(COMMAND perl run_test.pl)
