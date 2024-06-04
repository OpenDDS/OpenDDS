cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_tests_dcps)
enable_testing()

find_package(OpenDDS REQUIRED)

add_subdirectory(Compiler)
add_subdirectory(HelloWorld)
