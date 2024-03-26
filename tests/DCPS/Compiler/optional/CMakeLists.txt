cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_compiler_optional CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(${PROJECT_NAME}  "main.cpp")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME optional_annotation)
opendds_target_sources(${PROJECT_NAME} PUBLIC "optional.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${PROJECT_NAME} OpenDDS::Dcps gtest)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

configure_file(run_test.pl . COPYONLY)
opendds_add_test(COMMAND perl run_test.pl)
