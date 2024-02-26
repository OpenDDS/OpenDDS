cmake_minimum_required(VERSION 3.8...3.27)
project(opendds_idl_compiler_tests CXX)

find_package(OpenDDS REQUIRED)

_opendds_save_cache(OPENDDS_AUTO_LINK_DCPS BOOL ON)
_opendds_save_cache(OPENDDS_USE_CORRECT_INCLUDE_SCOPE BOOL ON)

macro(subtest name)
  set(no_value_options DUMMY)
  set(single_value_options)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  set(target "${PROJECT_NAME}_${name}")
  if(arg_DUMMY)
    add_custom_target(${target})
  else()
    add_library(${target})
  endif()
  set(idl_file "${CMAKE_CURRENT_BINARY_DIR}/${name}.idl")
  configure_file(test.idl "${idl_file}" COPYONLY)
endmacro()

function(remove_build_interface incs out_var)
  foreach(inc ${incs})
    string(REGEX REPLACE "\\$<BUILD_INTERFACE:" "" inc "${inc}")
    string(REGEX REPLACE ">\$" "" inc "${inc}")
    list(APPEND out "${inc}")
  endforeach()
  set("${out_var}" "${out}" PARENT_SCOPE)
endfunction()

function(assert_includes target public_and_interface public_and_private)
  get_target_property(interface_include_directories ${target} INTERFACE_INCLUDE_DIRECTORIES)
  remove_build_interface("${interface_include_directories}" interface_include_directories)
  foreach(inc ${public_and_interface})
    if(NOT "${inc}" IN_LIST interface_include_directories)
      message(SEND_ERROR "ERROR: ${target} expected ${inc} in public/interface includes")
    endif()
  endforeach()
  if(interface_include_directories)
    foreach(inc ${interface_include_directories})
      if(NOT "${inc}" IN_LIST public_and_interface)
        message(SEND_ERROR "ERROR: ${target} unexpected ${inc} in public/interface includes")
      endif()
    endforeach()
  endif()
  get_target_property(include_directories ${target} INCLUDE_DIRECTORIES)
  remove_build_interface("${include_directories}" include_directories)
  foreach(inc ${public_and_private})
    if(NOT ${inc} IN_LIST include_directories)
      message(SEND_ERROR "ERROR: ${target} expected ${inc} in public/private includes")
    endif()
  endforeach()
  if(include_directories)
    foreach(inc ${include_directories})
      if(NOT ${inc} IN_LIST public_and_private)
        message(SEND_ERROR "ERROR: ${target} unexpected ${inc} in public/private includes")
      endif()
    endforeach()
  endif()
endfunction()

set(opendds_gen "${CMAKE_CURRENT_BINARY_DIR}/opendds_generated")
# Directory has to exist for get_filename_component(REAPATH) to consistently
# resolve it.
file(MAKE_DIRECTORY "${opendds_gen}")
get_filename_component(opendds_gen "${opendds_gen}" REALPATH)

function(assert_generated_output file)
  set(no_value_options EXPECT_FAIL)
  set(single_value_options O_OPT INCLUDE_BASE EXPECT)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  set(args)
  if(arg_INCLUDE_BASE)
    list(APPEND args INCLUDE_BASE "${arg_INCLUDE_BASE}")
  endif()
  if(arg_O_OPT)
    list(APPEND args O_OPT "${arg_O_OPT}")
  endif()
  _opendds_get_generated_output(${target} "${file}"
    DIR_PATH_VAR dir_path
    FILE_PATH_VAR file_path
    PREFIX_PATH_VAR prefix_path
    FAIL_VAR failed
    ${args})

  if(arg_EXPECT_FAIL)
    set(expect_failed TRUE)
  else()
    set(expect_failed FALSE)
  endif()
  get_filename_component(filename "${file}" NAME)
  set(expect_dir_path "${arg_EXPECT}")
  set(expect_file_path "${expect_dir_path}/${filename}")
  get_filename_component(filename_no_ext "${file}" NAME_WE)
  set(expect_prefix_path "${expect_dir_path}/${filename_no_ext}")
  foreach(var failed dir_path file_path prefix_path)
    set(val "${${var}}")
    # message("${var} ${val}")
    set(expect "${expect_${var}}")
    if(NOT val STREQUAL expect)
      message(SEND_ERROR
        "  ERROR: in include base \"${arg_INCLUDE_BASE}\" the file:\n"
        "  \n"
        "    ${file}\n"
        "  \n"
        "  expected ${var} to be:\n"
        "  \n"
        "    ${expect}\n"
        "  \n"
        "  but it was:\n"
        "  \n"
        "    ${val}\n")
      return()
    endif()
    if(var STREQUAL "failed" AND val)
      # Don't check other values if it failed
      return()
    endif()
  endforeach()
endfunction()

subtest(generated_output DUMMY)
foreach(in "" "${opendds_gen}/")
  assert_generated_output("${in}file.ext" EXPECT "${opendds_gen}")
  assert_generated_output("${in}x/file.ext" EXPECT "${opendds_gen}")
  assert_generated_output("${in}x/y/file.ext" EXPECT "${opendds_gen}")
  assert_generated_output("${in}x/y/z/file.ext" EXPECT "${opendds_gen}")
  assert_generated_output("${in}file.ext" INCLUDE_BASE "." EXPECT "${opendds_gen}")
  assert_generated_output("${in}x/file.ext" INCLUDE_BASE "." EXPECT "${opendds_gen}/x")
  assert_generated_output("${in}x/y/file.ext" INCLUDE_BASE "." EXPECT "${opendds_gen}/x/y")
  assert_generated_output("${in}x/y/z/file.ext" INCLUDE_BASE "." EXPECT "${opendds_gen}/x/y/z")
  assert_generated_output("${in}x/file.ext" INCLUDE_BASE "x" EXPECT "${opendds_gen}")
  assert_generated_output("${in}x/y/file.ext" INCLUDE_BASE "x" EXPECT "${opendds_gen}/y")
  assert_generated_output("${in}x/y/z/file.ext" INCLUDE_BASE "x" EXPECT "${opendds_gen}/y/z")
  assert_generated_output("${in}x/y/file.ext" INCLUDE_BASE "x/y" EXPECT "${opendds_gen}")
  assert_generated_output("${in}x/y/z/file.ext" INCLUDE_BASE "x/y" EXPECT "${opendds_gen}/z")
endforeach()
assert_generated_output("../file.ext" INCLUDE_BASE "." EXPECT_FAIL)
assert_generated_output("x/file.ext" INCLUDE_BASE "x/y/z" EXPECT_FAIL)
assert_generated_output("o_opt_file.ext" O_OPT "o_opt_dir" EXPECT "${opendds_gen}/o_opt_dir")

subtest(no_scope)
opendds_target_sources(${target} ${idl_file})
assert_includes(${target} "" "${opendds_gen}")

subtest(public)
opendds_target_sources(${target} PUBLIC ${idl_file})
assert_includes(${target} "${opendds_gen}" "${opendds_gen}")

subtest(interface)
opendds_target_sources(${target} INTERFACE ${idl_file})
assert_includes(${target} "${opendds_gen}" "${opendds_gen}")

subtest(private)
opendds_target_sources(${target} PRIVATE ${idl_file})
assert_includes(${target} "" "${opendds_gen}")

subtest(o_opt)
opendds_target_sources(${target} ${idl_file}
  OPENDDS_IDL_OPTIONS -o opendds-idl-output
  TAO_IDL_OPTIONS -o tao-idl-output)
assert_includes(${target} "" "${opendds_gen}/opendds-idl-output;${opendds_gen}/tao-idl-output")

_opendds_restore_cache()
