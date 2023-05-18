# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/tao_idl_sources.cmake)

function(_opendds_add_to_interface_files_prop target kind file)
  set(prop_name "OPENDDS_${kind}_INTERFACE_FILES")
  get_target_property(interface_files ${target} ${prop_name})
  if(NOT interface_files)
    set(interface_files "")
  endif()
  if(NOT ${file} IN_LIST interface_files)
    list(APPEND interface_files ${file})
    set_target_properties(${target} PROPERTIES ${prop_name} "${interface_files}")
  endif()
endfunction()

function(_opendds_add_idl_or_header_files target scope is_generated files)
  if(is_generated)
    set(gen_kind "GENERATED")
  else()
    set(gen_kind "PASSED")
  endif()
  foreach(file ${files})
    if(NOT file)
      message(FATAL_ERROR "Invalid file in"
        "_opendds_add_idl_or_header_files(${target} ${scope} ${gen_kind} ${files})")
    endif()
    get_filename_component(file ${file} REALPATH)
    target_sources(${target} ${scope} $<BUILD_INTERFACE:${file}>)

    if(NOT ${scope} STREQUAL "PRIVATE")
      if(file MATCHES "\\.idl$")
        set(file_kind IDL)
      else()
        set(file_kind HEADER)
      endif()
      _opendds_add_to_interface_files_prop(${target} "${gen_kind}_${file_kind}" ${file})
      _opendds_add_to_interface_files_prop(${target} "ALL_${gen_kind}" ${file})
      _opendds_add_to_interface_files_prop(${target} "ALL_${file_kind}" ${file})
      _opendds_add_to_interface_files_prop(${target} "ALL" ${file})
    endif()
  endforeach()
endfunction()

function(_opendds_target_generated_dependencies target idl_file scope)
  get_source_file_property(idl_ts_files ${idl_file} OPENDDS_TYPESUPPORT_IDLS)
  set(all_idl_files ${idl_file})
  if(idl_ts_files)
    list(APPEND all_idl_files ${idl_ts_files})
  endif()

  foreach(file ${all_idl_files})
    get_source_file_property(cpps ${file} OPENDDS_CPP_FILES)
    if(cpps)
      list(APPEND cpp_files ${cpps})
    endif()
    get_source_file_property(hdrs ${file} OPENDDS_HEADER_FILES)
    if(hdrs)
      list(APPEND hdr_files ${hdrs})
    endif()
  endforeach()

  set(all_gen_files ${cpp_files} ${hdr_files})
  set(idl_or_header_gen_files ${hdr_files})
  if(idl_ts_files)
    list(APPEND all_gen_files ${idl_ts_files})
    list(APPEND idl_or_header_gen_files ${idl_ts_files})
  endif()
  set(all_files ${all_gen_files} ${idl_file})

  get_source_file_property(bridge_target ${idl_file} OPENDDS_IDL_BRIDGE_TARGET)
  if(NOT bridge_target)
    # Each IDL file corresponds to one bridge target. All targets which depend
    # upon the C/C++ files generated from IDL compilation will also depend upon
    # the bridge target to guarantee that IDL files will compile prior to the
    # dependent targets. This is simply set to the first IDL-Dependent target.
    set(bridge_target ${target})

    set_source_files_properties(${idl_file}
      PROPERTIES
        OPENDDS_IDL_BRIDGE_TARGET ${bridge_target})

    set_source_files_properties(${all_idl_files} ${hdr_files}
      PROPERTIES
        HEADER_FILE_ONLY ON)

    set_source_files_properties(${cpp_files} ${hdr_files}
      PROPERTIES
        SKIP_AUTOGEN ON)

    source_group("Generated Files" FILES ${all_gen_files})
    source_group("IDL Files" FILES ${idl_file})
  else()
    add_dependencies(${target} ${bridge_target})
  endif()

  _opendds_add_idl_or_header_files(${target} ${scope} TRUE "${idl_or_header_gen_files}")
  _opendds_add_idl_or_header_files(${target} ${scope} FALSE "${idl_file}")
  target_sources(${target} PRIVATE ${cpp_files})
endfunction()

function(_opendds_export_target_property target property_name)
  if(NOT ${CMAKE_VERSION} VERSION_LESS "3.12.0")
    get_property(target_export_properties TARGET ${target} PROPERTY "EXPORT_PROPERTIES")
    if(NOT property_name IN_LIST target_export_properties)
      list(APPEND target_export_properties ${property_name})
      set_property(TARGET ${target} PROPERTY "EXPORT_PROPERTIES" "${target_export_properties}")
    endif()
  endif()
endfunction()

function(_opendds_target_idl_sources target)
  set(one_value_args SCOPE SKIP_TAO_IDL AUTO_INCLUDES)
  set(multi_value_args TAO_IDL_FLAGS DDS_IDL_FLAGS IDL_FILES)
  cmake_parse_arguments(arg "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  # Language mappings used by the IDL files are mixed together with any
  # existing OPENDDS_LANGUAGE_MAPPINGS value on the target.
  get_property(all_mappings TARGET ${target}
    PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
  _opendds_export_target_property(${target} OPENDDS_LANGUAGE_MAPPINGS)

  set(all_auto_includes)
  foreach(idl_file ${arg_IDL_FILES})
    if(NOT IS_ABSOLUTE ${idl_file})
      set(idl_file ${CMAKE_CURRENT_LIST_DIR}/${idl_file})
    endif()

    get_property(generated_dependencies SOURCE ${idl_file}
      PROPERTY OPENDDS_IDL_GENERATED_DEPENDENCIES SET)

    if(generated_dependencies)
      # If an IDL-Generation command was already created this file can safely be
      # skipped; however, the dependencies still need to be added to the target.
      _opendds_target_generated_dependencies(${target} ${idl_file} ${arg_SCOPE})

      get_property(file_mappings SOURCE ${idl_file}
        PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
      list(APPEND all_mappings ${file_mappings})

      get_property(file_auto_includes SOURCE ${idl_file}
        PROPERTY OPENDDS_IDL_AUTO_INCLUDES)
      list(APPEND all_auto_includes ${file_auto_includes})
    else()
      list(APPEND non_generated_idl_files ${idl_file})
    endif()
  endforeach()

  if(NOT non_generated_idl_files)
    list(REMOVE_DUPLICATES all_mappings)
    set_property(TARGET ${target}
      PROPERTY OPENDDS_LANGUAGE_MAPPINGS "${all_mappings}")

    if(arg_AUTO_INCLUDES)
      list(REMOVE_DUPLICATES all_auto_includes)
      set("${arg_AUTO_INCLUDES}" "${all_auto_includes}" PARENT_SCOPE)
    endif()

    return()
  endif()

  get_property(target_link_libs TARGET ${target} PROPERTY LINK_LIBRARIES)
  if("OpenDDS::FACE" IN_LIST target_link_libs)
    foreach(tao_face_flag -SS -Wb,no_fixed_err)
      if(NOT "${arg_TAO_IDL_FLAGS}" MATCHES "${tao_face_flag}")
        list(APPEND arg_TAO_IDL_FLAGS ${tao_face_flag})
      endif()
    endforeach()

    foreach(opendds_face_flag -GfaceTS -Lface)
      if(NOT "${arg_DDS_IDL_FLAGS}" MATCHES "${opendds_face_flag}")
        list(APPEND arg_DDS_IDL_FLAGS ${opendds_face_flag})
      endif()
    endforeach()
  endif()

  file(RELATIVE_PATH working_dir ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})

  if(NOT IS_ABSOLUTE "${working_dir}")
    set(working_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${working_dir})
    set(working_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/${working_dir})
  else()
    set(working_binary_dir ${working_dir})
    set(working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  # remove trailing slashes
  string(REGEX REPLACE "/$" "" working_binary_dir ${working_binary_dir})
  string(REGEX REPLACE "/$" "" working_source_dir ${working_source_dir})

  # opendds_idl would generate different code with the -I flag followed by absolute path
  # or relative path, if it's a relative path we need to keep it a relative path to the binary tree
  file(RELATIVE_PATH rel_path_to_source_tree ${working_binary_dir} ${working_source_dir})
  if(rel_path_to_source_tree)
    set(rel_path_to_source_tree "${rel_path_to_source_tree}/")
  endif()

  foreach(flag ${arg_DDS_IDL_FLAGS})
    if("${flag}" MATCHES "^-I(\\.\\..*)")
      list(APPEND opendds_idl_opts "-I${rel_path_to_source_tree}${CMAKE_MATCH_1}")
    else()
      list(APPEND opendds_idl_opts ${flag})
    endif()
  endforeach()

  set(opendds_idl_opt_var_prefix "opendds_idl_opt")
  set(opendds_idl_no_value_opts "-SI" "-GfaceTS" "-Wb,java" "-Lc++11" "-Lface")
  set(opendds_idl_one_value_opts "-o")
  set(opendds_idl_multi_value_opts)
  foreach(opt ${opendds_idl_all_opts})
    unset("${opendds_idl_opt_var_prefix}_${opt}")
  endforeach()
  cmake_parse_arguments(opendds_idl_opt
    "${opendds_idl_no_value_opts}"
    "${opendds_idl_one_value_opts}"
    "${opendds_idl_multi_value_opts}"
    "${opendds_idl_opts}"
  )

  foreach(input ${non_generated_idl_files})
    get_filename_component(noext_name ${input} NAME_WE)
    get_filename_component(abs_filename ${input} ABSOLUTE)
    get_filename_component(file_ext ${input} EXT)
    get_filename_component(idl_file_dir ${abs_filename} DIRECTORY)

    _opendds_get_generated_idl_output(
      ${target} ${input} "${opendds_idl_opt_-o}" output_prefix output_dir)
    set(file_auto_includes "${output_dir}")

    set(type_support_idl_file)
    set(idl_files)
    set(generated_files)
    set(file_mappings)
    set(extra_options)
    set(type_support_idl_file)

    if(NOT opendds_idl_opt_-SI)
      set(type_support_idl_file "${output_prefix}TypeSupport.idl")
      list(APPEND idl_files ${type_support_idl_file})
      list(APPEND generated_files ${type_support_idl_file})
    endif()
    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_TYPESUPPORT_IDLS "${type_support_idl_file}")

    set(cpp_header_files "${output_prefix}TypeSupportImpl.h")
    set(cpp_source_files "${output_prefix}TypeSupportImpl.cpp")

    if(opendds_idl_opt_-GfaceTS)
      list(APPEND cpp_header_files "${output_prefix}_TS.hpp")
      list(APPEND cpp_source_files "${output_prefix}_TS.cpp")
    endif()

    if(opendds_idl_opt_-Lface)
      list(APPEND cpp_header_files "${output_prefix}C.h")
      list(APPEND file_mappings "FACE")
    elseif(opendds_idl_opt_-Lc++11)
      list(APPEND cpp_header_files "${output_prefix}C.h")
      list(APPEND file_mappings "C++11")
    else()
      list(APPEND idl_files ${input})
      list(APPEND file_mappings "C++03")
    endif()

    if(opendds_idl_opt_-Wb,java)
      set(java_list "${output_prefix}${file_ext}.TypeSupportImpl.java.list")
      set_property(SOURCE ${abs_filename} APPEND PROPERTY
        OPENDDS_JAVA_OUTPUTS "@${java_list}")
      list(APPEND generated_files ${java_list})
      list(APPEND extra_options "-j")
      list(APPEND file_mappings "Java")
    endif()

    list(APPEND generated_files ${cpp_header_files} ${cpp_source_files})

    _opendds_tao_append_runtime_lib_dir_to_path(extra_lib_dirs)

    add_custom_command(
      OUTPUT ${generated_files}
      DEPENDS opendds_idl "${DDS_ROOT}/dds/idl/IDLTemplate.txt"
      MAIN_DEPENDENCY ${abs_filename}
      COMMAND
        ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_INCLUDE_DIR}"
        "${extra_lib_dirs}"
        $<TARGET_FILE:opendds_idl> "-I${idl_file_dir}"
        ${opendds_idl_opts} ${extra_options} ${abs_filename} -o "${output_dir}"
    )

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_CPP_FILES ${cpp_source_files})

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_HEADER_FILES ${cpp_header_files})

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_LANGUAGE_MAPPINGS ${file_mappings})

    if(NOT arg_SKIP_TAO_IDL)
      _opendds_tao_idl(${target}
        IDL_FLAGS
          "-I${DDS_ROOT}"
          "-I${idl_file_dir}" # The type-support IDL will include the primary IDL file
          ${arg_TAO_IDL_FLAGS}
        IDL_FILES ${idl_files}
        AUTO_INCLUDES tao_idl_auto_includes
      )
      list(APPEND file_auto_includes "${tao_idl_auto_includes}")
    endif()

    set_property(SOURCE ${abs_filename} PROPERTY
      OPENDDS_IDL_GENERATED_DEPENDENCIES TRUE)

    list(REMOVE_DUPLICATES file_auto_includes)
    set_property(SOURCE ${abs_filename} PROPERTY
      OPENDDS_IDL_AUTO_INCLUDES "${file_auto_includes}")
    list(APPEND all_auto_includes "${file_auto_includes}")

    _opendds_target_generated_dependencies(${target} ${abs_filename} ${arg_SCOPE})

    list(APPEND all_mappings ${file_mappings})
    list(REMOVE_DUPLICATES all_mappings)
  endforeach()

  set_property(TARGET ${target}
    PROPERTY "OPENDDS_LANGUAGE_MAPPINGS" ${all_mappings})

  if(arg_AUTO_INCLUDES)
    list(REMOVE_DUPLICATES all_auto_includes)
    set("${arg_AUTO_INCLUDES}" "${all_auto_includes}" PARENT_SCOPE)
  endif()
endfunction()
