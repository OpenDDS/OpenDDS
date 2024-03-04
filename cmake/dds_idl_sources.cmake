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
    if(NOT file MATCHES "\\.idl$")
      target_sources(${target} PRIVATE $<BUILD_INTERFACE:${file}>)
    endif()

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
  set(one_value_args
    SCOPE
    SKIP_TAO_IDL
    SKIP_OPENDDS_IDL
    AUTO_INCLUDES
    INCLUDE_BASE
  )
  set(multi_value_args TAO_IDL_FLAGS DDS_IDL_FLAGS IDL_FILES)
  cmake_parse_arguments(arg "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  set(debug FALSE)
  if(opendds_target_sources IN_LIST OPENDDS_CMAKE_VERBOSE)
    set(debug TRUE)
  endif()

  # Language mappings used by the IDL files are mixed together with any
  # existing OPENDDS_LANGUAGE_MAPPINGS value on the target.
  get_property(all_mappings TARGET ${target}
    PROPERTY OPENDDS_LANGUAGE_MAPPINGS)
  _opendds_export_target_property(${target} OPENDDS_LANGUAGE_MAPPINGS)

  set(all_auto_includes)
  foreach(idl_file ${arg_IDL_FILES})
    get_property(generated_dependencies SOURCE ${idl_file}
      PROPERTY OPENDDS_IDL_GENERATED_DEPENDENCIES SET)

    # TODO: This breaks the OpenDDS build because must be able to call
    # opendds_target_sources on the same IDL files with different options.
    if(generated_dependencies AND FALSE)
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

  set(set_o_opt FALSE)
  set(o_opt)
  foreach(flag ${arg_DDS_IDL_FLAGS})
    if("${flag}" MATCHES "^-I(\\.\\..*)")
      list(APPEND opendds_idl_opts "-I${rel_path_to_source_tree}${CMAKE_MATCH_1}")
    elseif("${flag}" STREQUAL "-o")
      # Omit orignal -o* options because of https://github.com/DOCGroup/ACE_TAO/issues/2202
      set(set_o_opt TRUE)
    elseif(set_o_opt)
      set(o_opt "${flag}")
      set(set_o_opt FALSE)
    else()
      list(APPEND opendds_idl_opts "${flag}")
    endif()
  endforeach()

  set(opendds_idl_opt_var_prefix "opendds_idl_opt")
  set(opendds_idl_no_value_opts "-SI" "-GfaceTS" "-Wb,java" "-Lc++11" "-Lface")
  set(opendds_idl_one_value_opts "")
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

  _opendds_get_generated_output_dir(${target} gen_out)

  if(debug)
    message(STATUS "gen out: ${gen_out}")
    message(STATUS "IDL files:")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
  endif()
  foreach(input ${non_generated_idl_files})
    if(debug)
      string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" show_idl "${input}")
      message(STATUS "${show_idl}")
      list(APPEND CMAKE_MESSAGE_INDENT "  ")
    endif()
    get_filename_component(abs_filename ${input} ABSOLUTE)
    get_filename_component(file_ext ${input} EXT)
    get_filename_component(idl_file_dir ${abs_filename} DIRECTORY)

    set(idl_files)
    set(ts_idl_files)
    set(h_files)
    set(cpp_files)
    set(run_tao_idl_on_input FALSE)
    set(file_auto_includes "${gen_out}")
    set(file_mappings)
    set(tao_idl_opts ${arg_TAO_IDL_FLAGS})
    set(generated_files)
    set(extra_options)

    if(arg_SKIP_OPENDDS_IDL)
      set(run_tao_idl_on_input TRUE)
      if(debug)
        message(STATUS "SKIP_OPENDDS_IDL")
      endif()
    else()
      _opendds_get_generated_output(${target} "${input}"
        INCLUDE_BASE "${arg_INCLUDE_BASE}" O_OPT "${o_opt}" MKDIR
        PREFIX_PATH_VAR output_prefix DIR_PATH_VAR output_dir)
      _opendds_get_generated_output_dir(${target} file_auto_includes O_OPT "${o_opt}")

      if(NOT opendds_idl_opt_-SI)
        set(type_support_idl_file "${output_prefix}TypeSupport.idl")
        list(APPEND ts_idl_files ${type_support_idl_file})
        list(APPEND generated_files ${type_support_idl_file})
        set_property(SOURCE ${abs_filename} APPEND PROPERTY
          OPENDDS_TYPESUPPORT_IDLS "${type_support_idl_file}")
      endif()

      list(APPEND h_files "${output_prefix}TypeSupportImpl.h")
      list(APPEND cpp_files "${output_prefix}TypeSupportImpl.cpp")

      if(opendds_idl_opt_-GfaceTS)
        list(APPEND h_files "${output_prefix}_TS.hpp")
        list(APPEND cpp_files "${output_prefix}_TS.cpp")
      endif()

      if(opendds_idl_opt_-Lface)
        list(APPEND h_files "${output_prefix}C.h")
        list(APPEND file_mappings "FACE")
      elseif(opendds_idl_opt_-Lc++11)
        list(APPEND h_files "${output_prefix}C.h")
        list(APPEND file_mappings "C++11")
      else()
        set(run_tao_idl_on_input TRUE)
      endif()

      if(opendds_idl_opt_-Wb,java)
        set(java_list "${output_prefix}${file_ext}.TypeSupportImpl.java.list")
        set_property(SOURCE ${abs_filename} APPEND PROPERTY
          OPENDDS_JAVA_OUTPUTS "@${java_list}")
        list(APPEND generated_files ${java_list})
        list(APPEND extra_options "-j")
        list(APPEND file_mappings "Java")
      endif()

      list(APPEND generated_files ${h_files} ${cpp_files})

      _opendds_tao_append_runtime_lib_dir_to_path(extra_lib_dirs)

      set(opendds_idl_args
        "-I${idl_file_dir}"
        ${opendds_idl_opts} ${extra_options}
        ${abs_filename} -o "${output_dir}"
      )
      if(debug)
        message(STATUS "opendds_idl ${opendds_idl_args}")
        foreach(generated_file ${generated_files})
          string(REPLACE "${output_dir}/" "" generated_file "${generated_file}")
          message(STATUS "${generated_file}")
        endforeach()
      endif()
      add_custom_command(
        OUTPUT ${generated_files}
        DEPENDS OpenDDS::opendds_idl "${DDS_ROOT}/dds/idl/IDLTemplate.txt"
        MAIN_DEPENDENCY ${abs_filename}
        COMMAND
          ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_INCLUDE_DIR}"
          "${extra_lib_dirs}" $<TARGET_FILE:OpenDDS::opendds_idl> ${opendds_idl_args}
      )

      list(APPEND tao_idl_opts
        "-I${DDS_ROOT}"
        "-I${idl_file_dir}" # The type-support IDL will include the primary IDL file
      )

      set_property(SOURCE ${abs_filename} APPEND PROPERTY OPENDDS_CPP_FILES ${cpp_files})
      set_property(SOURCE ${abs_filename} APPEND PROPERTY OPENDDS_HEADER_FILES ${h_files})
    endif()

    if(NOT arg_SKIP_TAO_IDL)
      if(run_tao_idl_on_input)
        list(APPEND idl_files ${input})
        list(APPEND file_mappings "C++03")
      endif()
      if(idl_files)
        _opendds_tao_idl(${target}
          IDL_FLAGS ${tao_idl_opts}
          IDL_FILES ${idl_files}
          INCLUDE_BASE "${include_base}"
          AUTO_INCLUDES tao_idl_auto_includes
        )
        list(APPEND file_auto_includes "${tao_idl_auto_includes}")
      endif()
      if(ts_idl_files)
        _opendds_tao_idl(${target}
          IDL_FLAGS ${tao_idl_opts}
          IDL_FILES ${ts_idl_files}
          INCLUDE_BASE "${gen_out}" # Generated files must be relative to generated output
          AUTO_INCLUDES tao_idl_ts_auto_includes
        )
        list(APPEND file_auto_includes "${tao_idl_auto_includes}")
      endif()
    elseif(arg_SKIP_TAO_IDL)
      if(debug)
        message(STATUS "SKIP_TAO_IDL")
      endif()
    elseif(NOT idl_files)
      if(debug)
        message(STATUS "No IDL files for tao_idl")
      endif()
    endif()

    set_property(SOURCE ${abs_filename} PROPERTY
      OPENDDS_IDL_GENERATED_DEPENDENCIES TRUE)

    list(REMOVE_DUPLICATES file_auto_includes)
    set_property(SOURCE ${abs_filename} PROPERTY
      OPENDDS_IDL_AUTO_INCLUDES "${file_auto_includes}")
    list(APPEND all_auto_includes "${file_auto_includes}")

    _opendds_target_generated_dependencies(${target} ${abs_filename} ${arg_SCOPE})

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_LANGUAGE_MAPPINGS ${file_mappings})

    list(APPEND all_mappings ${file_mappings})
    list(REMOVE_DUPLICATES all_mappings)

    if(debug)
      _opendds_pop_list(CMAKE_MESSAGE_INDENT)
    endif()
  endforeach()
  if(debug)
    _opendds_pop_list(CMAKE_MESSAGE_INDENT)
  endif()

  set_property(TARGET ${target}
    PROPERTY "OPENDDS_LANGUAGE_MAPPINGS" ${all_mappings})

  if(arg_AUTO_INCLUDES)
    list(REMOVE_DUPLICATES all_auto_includes)
    set("${arg_AUTO_INCLUDES}" "${all_auto_includes}" PARENT_SCOPE)
  endif()
endfunction()
