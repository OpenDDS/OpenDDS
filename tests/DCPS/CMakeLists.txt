cmake_minimum_required(VERSION 3.3...4.0)
project(opendds_tests_dcps)
enable_testing()

find_package(OpenDDS REQUIRED)

add_subdirectory(Compiler)
add_subdirectory(HelloWorld)
