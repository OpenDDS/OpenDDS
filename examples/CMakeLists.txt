cmake_minimum_required(VERSION 3.28...4.0)
project(opendds_examples)
enable_testing()

find_package(OpenDDS REQUIRED)

add_subdirectory(DCPS)
