cmake_minimum_required(VERSION 3.8...3.27)

project(opendds_optional CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

# IDL Library
set(idl ${PROJECT_NAME}_idl)
add_library(${idl})
opendds_target_sources(${idl} PUBLIC "optional.idl" OPENDDS_IDL_OPTIONS "-Lc++11")
target_link_libraries(${idl} PUBLIC OpenDDS::Dcps)
target_compile_features(${idl} PUBLIC cxx_std_17)

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp # For run_test.pl
  OpenDDS::Rtps OpenDDS::Rtps_Udp # For run_test.pl --rtps
  ${idl}
)

set(subscriber ${PROJECT_NAME}_subscriber)
add_executable(${subscriber} "subscriber.cpp")
target_link_libraries(${subscriber} ${opendds_libs})
set_target_properties(${subscriber} PROPERTIES OUTPUT_NAME subscriber)
target_compile_features(${subscriber} PUBLIC cxx_std_17)

set(publisher ${PROJECT_NAME}_publisher)
add_executable(${publisher} "publisher.cpp")
target_link_libraries(${publisher} ${opendds_libs})
set_target_properties(${publisher} PROPERTIES OUTPUT_NAME publisher)
target_compile_features(${publisher} PUBLIC cxx_std_17)

configure_file(run_test.pl . COPYONLY)
configure_file(rtps.ini . COPYONLY)
opendds_add_test(NAME info_repo COMMAND perl run_test.pl)
opendds_add_test(NAME rtps COMMAND perl run_test.pl ini=rtps.ini)
