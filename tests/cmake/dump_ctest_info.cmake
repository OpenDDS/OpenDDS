# This seems like the easiest way to pull the info we need out of the CMake
# binary directory. This is run using CMake in script mode (-P), presumingly in
# a similar manner as ctest does, but with our own function definitions.
# Run from the binary/build directory where ctest was run.

# This basically makes opendds_dump_ctest_info_stack a global variable, but
# we have to use set(CACHE) after modifying the variable locally to make the
# change globally.
set(opendds_dump_ctest_info_stack "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")

function(add_test)
  message(STATUS "DUMP[TEST]: ${ARGN}")
endfunction()

function(set_tests_properties)
  message(STATUS "DUMP[SET_TESTS_PROPERTIES]: ${ARGN}")
endfunction()

function(subdirs)
  # Get the directory the calling source file is supposed to be in.
  list(LENGTH opendds_dump_ctest_info_stack opendds_child_index)
  math(EXPR opendds_current_dir_index "${opendds_child_index} - 1")
  list(GET opendds_dump_ctest_info_stack "${opendds_current_dir_index}" opendds_current_dir)

  # For each argument passed (it probably only ever going to be one, but this
  # is safer), push the directory onto the stack and include the test file in
  # that directory to dump the info it contains.
  foreach(opendds_dir "${ARGN}")
    if(NOT IS_ABSOLUTE "${opendds_dir}")
      set(opendds_dir "${opendds_current_dir}/${opendds_dir}")
    endif()
    set(opendds_file "${opendds_dir}/CTestTestfile.cmake")
    message(STATUS "DUMP[START_SUBDIRS]: ${opendds_file}")
    list(APPEND opendds_dump_ctest_info_stack "${opendds_dir}")
    set(opendds_dump_ctest_info_stack "${opendds_dump_ctest_info_stack}" CACHE INTERNAL "")
    include("${opendds_file}")
    list(REMOVE_AT opendds_dump_ctest_info_stack ${opendds_child_index})
    set(opendds_dump_ctest_info_stack "${opendds_dump_ctest_info_stack}" CACHE INTERNAL "")
    message(STATUS "DUMP[END_SUBDIRS]: ${opendds_file}")
  endforeach()
endfunction()

subdirs("${CMAKE_CURRENT_BINARY_DIR}")
