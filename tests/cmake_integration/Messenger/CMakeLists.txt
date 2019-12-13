cmake_minimum_required(VERSION 3.3)

project(opendds_cmake_messenger_tests)

# Must Specify binary_dir Option to Avoid Conflicts
add_subdirectory(Messenger_1 Messenger_1_build)
add_subdirectory(Messenger_2 Messenger_2_build)
