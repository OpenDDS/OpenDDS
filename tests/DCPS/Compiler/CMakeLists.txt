cmake_minimum_required(VERSION 3.3...3.27)
project(opendds_tests_dcps_compiler)
enable_testing()

find_package(OpenDDS REQUIRED)

add_subdirectory(char_literals)
add_subdirectory(optional)
add_subdirectory(xcdr)
