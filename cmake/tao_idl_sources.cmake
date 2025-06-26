# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

function(_opendds_compile_idl compiler idl_file)
  set(no_value_options)
  set(single_value_options)
  set(multi_value_options OUTPUT DEPENDS OPTS)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  # Add TAO lib directory to path
  if(MSVC)
    set(env_var_name PATH)
  else()
    set(env_var_name LD_LIBRARY_PATH)
  endif()
  set(path_list "$ENV{${env_var_name}}" "${TAO_LIB_DIR}")
  if(TARGET OpenDDS::Util)
    list(APPEND path_list "$<TARGET_FILE_DIR:OpenDDS::Util>")
  endif()
  _opendds_path_list(path_list ${path_list})
  if(NOT MSVC)
    string(REPLACE "\\" "/" path_list "${path_list}")
  endif()

  # If supported, make this as part of the codegen target.
  set(extra)
  if(POLICY CMP0171)
    cmake_policy(SET CMP0171 NEW)
    list(APPEND extra CODEGEN)
  endif()

  add_custom_command(
    OUTPUT ${arg_OUTPUT}
    MAIN_DEPENDENCY "${idl_file}"
    DEPENDS ${arg_DEPENDS}
    ${extra}
    COMMAND ${CMAKE_COMMAND} -E env
      "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_INCLUDE_DIR}" "${env_var_name}=${path_list}"
      "${compiler}" ${arg_OPTS} "${idl_file}"
  )
endfunction()

function(_opendds_tao_idl target)
  set(one_value_args
    INCLUDE_BASE
    AUTO_INCLUDES_VAR
    H_FILES_VAR
    CPP_FILES_VAR
  )
  set(multi_value_args IDL_FLAGS IDL_FILES)
  cmake_parse_arguments(arg "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(NOT arg_IDL_FILES)
    message(FATAL_ERROR "called _opendds_tao_idl(${target}) without specifying IDL_FILES")
  endif()

  set(working_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})

  # convert all include paths to be relative to binary tree instead of to source tree
  file(RELATIVE_PATH rel_path_to_source_tree ${working_binary_dir} ${working_source_dir})
  set(remove_next_opt FALSE)
  set(converted_flags)
  foreach(flag ${arg_IDL_FLAGS})
    if("${flag}" MATCHES "^-I(\\.\\..*)")
      list(APPEND converted_flags "-I${rel_path_to_source_tree}/${CMAKE_MATCH_1}")
    elseif("${flag}" MATCHES "^-o[SA]?$")
      # Omit orignal -o* options because of https://github.com/DOCGroup/ACE_TAO/issues/2202
      set(remove_next_opt TRUE)
    elseif(remove_next_opt)
      set(remove_next_opt FALSE)
    else()
      list(APPEND converted_flags ${flag})
      # if the flag is like "-Wb,stub_export_file=filename" then set the variable
      # "idl_cmd_arg-wb-stub_export_file" to filename
      string(REGEX MATCH "^-Wb,([^=]+)=(.+)" m "${flag}")
      if(m)
        set(idl_cmd_arg-wb-${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
      endif()
    endif()
  endforeach()

  set(option_args -Sch -Sci -Scc -Ssh -SS -GA -GT -GX -Gxhst -Gxhsk)
  cmake_parse_arguments(idl_cmd_arg "${option_args}" "-o;-oS;-oA" "" ${arg_IDL_FLAGS})

  set(feature_flags)
  if(OPENDDS_TAO_CORBA_E_MICRO)
    list(APPEND feature_flags -DCORBA_E_MICRO -Gce)
  endif()
  if(OPENDDS_TAO_CORBA_E_COMPACT)
    list(APPEND feature_flags -DCORBA_E_COMPACT -Gce)
  endif()
  if(OPENDDS_TAO_MINIMUM_CORBA)
    list(APPEND feature_flags -DTAO_HAS_MINIMUM_POA -Gmc)
  endif()
  if(OPENDDS_TAO_NO_IIOP)
    list(APPEND feature_flags -DTAO_LACKS_IIOP)
  endif()
  if(OPENDDS_TAO_GEN_OSTREAM)
    list(APPEND feature_flags -Gos)
  endif()
  if(NOT OPENDDS_TAO_OPTIMIZE_COLLOCATED_INVOCATIONS)
    list(APPEND feature_flags -Sp -Sd)
  endif()

  set(auto_includes)
  if(arg_INCLUDE_BASE)
    list(APPEND converted_flags "-I${arg_INCLUDE_BASE}")
  endif()

  set(all_h_files)
  set(all_cpp_files)
  foreach(idl_file ${arg_IDL_FILES})
    set(added_output_args)
    _opendds_get_generated_output(${target} "${idl_file}"
      INCLUDE_BASE "${arg_INCLUDE_BASE}" O_OPT "${idl_cmd_arg_-o}" MKDIR
      PREFIX_PATH_VAR output_prefix DIR_PATH_VAR output_dir)
    list(APPEND auto_includes "${output_dir}")
    list(APPEND added_output_args "-o" "${output_dir}")
    if(idl_cmd_arg_-oS)
      _opendds_get_generated_output(${target} "${idl_file}"
        INCLUDE_BASE "${arg_INCLUDE_BASE}" O_OPT "${idl_cmd_arg_-oS}" MKDIR
        PREFIX_PATH_VAR skel_output_prefix DIR_PATH_VAR skel_output_dir)
      list(APPEND auto_includes "${skel_output_dir}")
      list(APPEND added_output_args "-oS" "${skel_output_dir}")
    else()
      set(skel_output_prefix "${output_prefix}")
    endif()
    if(idl_cmd_arg_-oA)
      _opendds_get_generated_output(${target} "${idl_file}"
      INCLUDE_BASE "${arg_INCLUDE_BASE}" O_OPT "${idl_cmd_arg_-oA}" MKDIR
        PREFIX_PATH_VAR anyop_output_prefix DIR_PATH_VAR anyop_output_dir)
      list(APPEND auto_includes "${anyop_output_dir}")
      list(APPEND added_output_args "-oA" "${anyop_output_dir}")
    else()
      set(anyop_output_prefix "${output_prefix}")
    endif()

    unset(stub_header_files)
    unset(skel_header_files)
    if(NOT idl_cmd_arg_-Sch)
      set(stub_header_files "${output_prefix}C.h")
    endif()

    if(NOT idl_cmd_arg_-Sci)
      list(APPEND stub_header_files "${output_prefix}C.inl")
    endif()

    if(NOT idl_cmd_arg_-Scc)
      set(stub_cpp_files "${output_prefix}C.cpp")
    endif()

    if(NOT idl_cmd_arg_-Ssh)
      set(skel_header_files "${skel_output_prefix}S.h")
    endif()

    if(NOT idl_cmd_arg_-SS)
      set(skel_cpp_files "${skel_output_prefix}S.cpp")
    endif()

    if(idl_cmd_arg_-GA)
      set(anyop_header_files "${anyop_output_prefix}A.h")
      set(anyop_cpp_files "${anyop_output_prefix}A.cpp")
    elseif(idl_cmd_arg_-GX)
      set(anyop_header_files "${anyop_output_prefix}A.h")
    endif()

    if(idl_cmd_arg_-GT)
      list(APPEND skel_header_files
        "${skel_output_prefix}S_T.h"
        "${skel_output_prefix}S_T.cpp")
    endif()

    if(idl_cmd_arg_-Gxhst AND DEFINED idl_cmd_arg-wb-stub_export_file)
      list(APPEND stub_header_files "${CMAKE_CURRENT_BINARY_DIR}/${idl_cmd_arg-wb-stub_export_file}")
    endif()

    if(idl_cmd_arg_-Gxhsk AND DEFINED idl_cmd_arg-wb-skel_export_file)
      list(APPEND skel_header_files "${CMAKE_CURRENT_BINARY_DIR}/${idl_cmd_arg-wb-skel_export_file}")
    endif()

    get_filename_component(idl_file_path "${idl_file}" ABSOLUTE)

    set(gperf_location $<TARGET_FILE:ACE::ace_gperf>)
    if(CMAKE_CONFIGURATION_TYPES)
      get_target_property(is_gperf_imported ACE::ace_gperf IMPORTED)
      if(is_gperf_imported)
        set(gperf_location $<TARGET_PROPERTY:ACE::ace_gperf,LOCATION>)
      endif(is_gperf_imported)
    endif()

    if(BUILD_SHARED_LIB AND TARGET TAO_IDL_BE)
      set(tao_idl_shared_libs TAO_IDL_BE TAO_IDL_FE)
    endif()

    set(tao_idl "$<TARGET_FILE:TAO::tao_idl>")
    if(CMAKE_GENERATOR STREQUAL "Ninja" AND TAO_IS_BEING_BUILT)
      if(CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "Using Ninja to build ACE/TAO requires CMake 3.24 or later. "
         "Please build ACE/TAO separately, use a newer CMake, or a different CMake generator.")
      else()
        set(tao_idl "$<PATH:ABSOLUTE_PATH,${tao_idl},\${cmake_ninja_workdir}>")
        set(gperf_location "$<PATH:ABSOLUTE_PATH,${gperf_location},\${cmake_ninja_workdir}>")
      endif()
    endif()
    set(tao_idl_args
      -g ${gperf_location} ${feature_flags} -Sg
      -Wb,pre_include=ace/pre.h -Wb,post_include=ace/post.h
      --idl-version 4 -as --unknown-annotations ignore
      -I${TAO_INCLUDE_DIR} -I${working_source_dir}
      ${converted_flags}
      ${added_output_args}
    )
    set(h_files
      ${stub_header_files}
      ${skel_header_files}
      ${anyop_header_files}
    )
    list(APPEND all_h_files ${h_files})
    set(cpp_files
      ${stub_cpp_files}
      ${skel_cpp_files}
      ${anyop_cpp_files}
    )
    list(APPEND all_cpp_files ${cpp_files})
    set(generated_files ${h_files} ${cpp_files})
    if(debug)
      message(STATUS "tao_idl ${tao_idl_args}")
      foreach(generated_file ${generated_files})
        string(REPLACE "${output_dir}/" "" generated_file "${generated_file}")
        string(REPLACE "${skel_output_dir}/" "" generated_file "${generated_file}")
        string(REPLACE "${anyop_output_dir}/" "" generated_file "${generated_file}")
        message(STATUS "${generated_file}")
      endforeach()
    endif()
    _opendds_compile_idl("${tao_idl}" "${idl_file_path}"
      OUTPUT ${generated_files}
      OPTS ${tao_idl_args}
      DEPENDS TAO::tao_idl ${tao_idl_shared_libs} ACE::ace_gperf
    )
  endforeach()

  if(arg_H_FILES_VAR)
    set("${arg_H_FILES_VAR}" "${h_files}" PARENT_SCOPE)
  endif()

  if(arg_CPP_FILES_VAR)
    set("${arg_CPP_FILES_VAR}" "${cpp_files}" PARENT_SCOPE)
  endif()

  if(arg_AUTO_INCLUDES_VAR)
    set("${arg_AUTO_INCLUDES_VAR}" "${auto_includes}" PARENT_SCOPE)
  endif()
endfunction()
