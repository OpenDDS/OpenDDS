cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_tests_inspect CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(${PROJECT_NAME} "writer.cpp")
opendds_target_sources(${PROJECT_NAME} PUBLIC "inspect.idl" OPENDDS_IDL_OPTIONS "-Gxtypes-complete")
set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME writer
)
target_link_libraries(${PROJECT_NAME} OpenDDS::Dcps OpenDDS::Rtps_Udp)

# configure_file(run_test.pl . COPYONLY)
# opendds_add_test(COMMAND perl run_test.pl cpp11 EXTRA_LIB_DIRS "${idl_dir}")
