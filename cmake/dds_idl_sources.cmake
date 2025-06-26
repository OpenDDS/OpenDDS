# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/tao_idl_sources.cmake)

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
  set(opendds_idl_no_value_opts
    "-SI"
    "-GfaceTS"
    "-Wb,java"
    "-Lc++11"
    "-Lface"
    "-Lspcpp"
  )
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
  foreach(arg IN LISTS opendds_idl_opt_UNPARSED_ARGUMENTS)
    if(arg MATCHES "^-L")
      message(FATAL_ERROR "Unknown lanaguage mapping: ${arg}")
    endif()
  endforeach()

  _opendds_get_generated_output_dir(${target} gen_out)

  if(debug)
    message(STATUS "gen out: ${gen_out}")
    message(STATUS "IDL files:")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
  endif()
  foreach(input IN LISTS arg_IDL_FILES)
    if(debug)
      string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" show_idl "${input}")
      message(STATUS "${show_idl}")
      list(APPEND CMAKE_MESSAGE_INDENT "  ")
    endif()
    get_filename_component(abs_filename ${input} ABSOLUTE)
    get_filename_component(file_ext ${input} EXT)
    get_filename_component(idl_file_dir ${abs_filename} DIRECTORY)

    set(idl_files "${input}")
    set(ts_idl_file)
    set(h_files)
    set(cpp_files)
    set(run_tao_idl_on_input FALSE)
    set(file_auto_includes "${gen_out}")
    set(file_mappings)
    set(tao_idl_opts ${arg_TAO_IDL_FLAGS})
    set(opendds_idl_generated_files)
    set(tao_idl_generated_files)
    set(generated_idl_or_header_files)
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
        set(ts_idl_file "${output_prefix}TypeSupport.idl")
        list(APPEND idl_files "${ts_idl_file}")
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
      elseif(opendds_idl_opt_-Lspcpp)
        list(APPEND h_files "${output_prefix}C.h")
        list(APPEND file_mappings "SPCPP")
      else()
        set(run_tao_idl_on_input TRUE)
      endif()

      set(opendds_idl_generated_files ${ts_idl_file} ${h_files} ${cpp_files})
      set(generated_idl_or_header_files ${ts_idl_file} ${h_files})

      if(opendds_idl_opt_-Wb,java)
        # set(java_list "${output_prefix}${file_ext}.TypeSupportImpl.java.list")
        # list(APPEND opendds_idl_generated_files ${java_list})
        # list(APPEND extra_options "-j")
        list(APPEND file_mappings "Java")
      endif()

      set(opendds_idl_args
        "-I${idl_file_dir}"
        ${opendds_idl_opts} ${extra_options}
        -o "${output_dir}"
      )
      if(debug)
        message(STATUS "opendds_idl ${opendds_idl_args}")
        foreach(generated_file ${opendds_idl_generated_files})
          string(REPLACE "${output_dir}/" "" generated_file "${generated_file}")
          message(STATUS "${generated_file}")
        endforeach()
      endif()
      _opendds_compile_idl($<TARGET_FILE:OpenDDS::opendds_idl> "${abs_filename}"
        OUTPUT ${opendds_idl_generated_files}
        OPTS ${opendds_idl_args}
        DEPENDS OpenDDS::opendds_idl "${DDS_ROOT}/dds/idl/IDLTemplate.txt"
      )

      list(APPEND tao_idl_opts
        "-I${idl_file_dir}" # The type-support IDL will include the primary IDL file
      )
      foreach(include_dir IN LISTS OPENDDS_INCLUDE_DIRS)
        list(APPEND tao_idl_opts "-I${include_dir}")
      endforeach()
    endif()

    if(NOT arg_SKIP_TAO_IDL)
      if(run_tao_idl_on_input)
        list(APPEND file_mappings "C++03")
        _opendds_tao_idl(${target}
          IDL_FLAGS ${tao_idl_opts}
          IDL_FILES ${input}
          INCLUDE_BASE "${include_base}"
          AUTO_INCLUDES_VAR tao_idl_auto_includes
          H_FILES_VAR tao_idl_h_files
          CPP_FILES_VAR tao_idl_cpp_files
        )
        list(APPEND file_auto_includes ${tao_idl_auto_includes})
        list(APPEND h_files ${tao_idl_h_files})
        list(APPEND cpp_files ${tao_idl_cpp_files})
        list(APPEND tao_idl_generated_files ${tao_idl_h_files} ${tao_idl_cpp_files})
        list(APPEND generated_idl_or_header_files ${tao_idl_h_files})
      endif()
      if(ts_idl_file)
        _opendds_tao_idl(${target}
          IDL_FLAGS ${tao_idl_opts}
          IDL_FILES ${ts_idl_file}
          INCLUDE_BASE "${gen_out}" # Generated files must be relative to generated output
          AUTO_INCLUDES_VAR tao_idl_ts_auto_includes
          H_FILES_VAR tao_idl_ts_h_files
          CPP_FILES_VAR tao_idl_ts_cpp_files
        )
        list(APPEND file_auto_includes ${tao_idl_ts_auto_includes})
        list(APPEND h_files ${tao_idl_ts_h_files})
        list(APPEND cpp_files ${tao_idl_ts_cpp_files})
        list(APPEND tao_idl_generated_files ${tao_idl_ts_h_files} ${tao_idl_ts_cpp_files})
        list(APPEND generated_idl_or_header_files ${tao_idl_ts_h_files})
      endif()
    elseif(debug)
      message(STATUS "SKIP_TAO_IDL")
    endif()

    list(REMOVE_DUPLICATES file_auto_includes)
    list(APPEND all_auto_includes "${file_auto_includes}")

    set(generated_files ${opendds_idl_generated_files} ${tao_idl_generated_files})

    # IDL compilation depends on custom targets to guarantee that IDL files
    # will compile prior to the dependent target. The target used to depend on
    # the files directly and other targets depending on that target, but
    # https://cmake.org/cmake/help/latest/policy/CMP0154.html broke the
    # compilation in Ninja by trying to compile C++ files before IDL files.
    # This method also allows for multiple targets to use the same IDL file
    # with different options.
    get_target_property(idl_file_count ${target} _OPENDDS_IDL_FILE_COUNT)
    if(NOT idl_file_count)
      set(idl_file_count 0)
    endif()
    math(EXPR idl_file_count "${idl_file_count} + 1")
    set_target_properties(${target} PROPERTIES _OPENDDS_IDL_FILE_COUNT ${idl_file_count})
    set(idl_target "_opendds_codegen_${idl_file_count}_for_${target}")
    add_custom_target(${idl_target} DEPENDS ${generated_files})
    add_dependencies(${target} ${idl_target})

    set_source_files_properties(${idl_files} ${h_files}
      PROPERTIES
        HEADER_FILE_ONLY ON)
    set_source_files_properties(${h_files} ${cpp_files}
      PROPERTIES
        SKIP_AUTOGEN ON)
    source_group("IDL Files" FILES ${idl_file})
    source_group("Generated Files" FILES ${generated_files})

    _opendds_add_idl_or_header_files(${target} ${arg_SCOPE} TRUE "${generated_idl_or_header_files}")
    _opendds_add_idl_or_header_files(${target} ${arg_SCOPE} FALSE "${input}")
    target_sources(${target} PRIVATE ${cpp_files})

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
