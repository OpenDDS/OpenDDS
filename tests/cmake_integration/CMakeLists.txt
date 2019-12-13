cmake_minimum_required(VERSION 3.3)

project(opendds_cmake_tests_root_project)

# Must Specify binary_dir Option to Avoid Conflicts
add_subdirectory(idl_include_path idl_include_path_build)
add_subdirectory(Messenger Messenger_build)
