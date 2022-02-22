# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/tao_idl_sources.cmake)

function(opendds_add_to_interface_files_prop target kind file)
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

function(opendds_add_idl_or_header_files target scope is_generated files)
  if(is_generated)
    set(gen_kind "GENERATED")
  else()
    set(gen_kind "PASSED")
  endif()
  foreach(file ${files})
    get_filename_component(file ${file} REALPATH)
    target_sources(${target} ${scope} $<BUILD_INTERFACE:${file}>)

    if(NOT ${scope} STREQUAL "PRIVATE")
      if(file MATCHES "\\.idl$")
        set(file_kind IDL)
      else()
        set(file_kind HEADER)
      endif()
      opendds_add_to_interface_files_prop(${target} "${gen_kind}_${file_kind}" ${file})
      opendds_add_to_interface_files_prop(${target} "ALL_${gen_kind}" ${file})
      opendds_add_to_interface_files_prop(${target} "ALL_${file_kind}" ${file})
      opendds_add_to_interface_files_prop(${target} "ALL" ${file})
    endif()
  endforeach()
endfunction()

function(opendds_target_generated_dependencies target idl_file scope)
  get_source_file_property(idl_ts_files ${idl_file} OPENDDS_TYPESUPPORT_IDLS)
  set(all_idl_files ${idl_file} ${idl_ts_files})

  foreach(file ${all_idl_files})
    get_source_file_property(cpps ${file} OPENDDS_CPP_FILES)
    get_source_file_property(hdrs ${file} OPENDDS_HEADER_FILES)
    list(APPEND cpp_files ${cpps})
    list(APPEND hdr_files ${hdrs})
  endforeach()

  set(all_gen_files ${cpp_files} ${hdr_files} ${idl_ts_files})
  set(all_files ${all_gen_files} ${idl_file})

  get_source_file_property(bridge_target ${idl_file} OPENDDS_IDL_BRIDGE_TARGET)
  if (NOT bridge_target)
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

  opendds_add_idl_or_header_files(${target} ${scope} TRUE "${idl_ts_files};${hdr_files}")
  opendds_add_idl_or_header_files(${target} ${scope} FALSE "${idl_file}")
  target_sources(${target} PRIVATE ${cpp_files})
endfunction()

function(opendds_export_target_property target property_name)
  if(NOT ${CMAKE_VERSION} VERSION_LESS "3.12.0")
    get_property(target_export_properties TARGET ${target} PROPERTY "EXPORT_PROPERTIES")
    if(NOT property_name IN_LIST target_export_properties)
      list(APPEND target_export_properties ${property_name})
      set_property(TARGET ${target} PROPERTY "EXPORT_PROPERTIES" "${target_export_properties}")
    endif()
  endif()
endfunction()

function(opendds_target_idl_sources target)
  set(oneValueArgs SCOPE SKIP_TAO_IDL)
  set(multiValueArgs TAO_IDL_FLAGS DDS_IDL_FLAGS IDL_FILES)
  cmake_parse_arguments(_arg "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Language mappings used by the IDL files are mixed together with any
  # existing OPENDDS_LANGUAGE_MAPPINGS value on the target.
  get_property(language_mappings TARGET ${target}
    PROPERTY "OPENDDS_LANGUAGE_MAPPINGS")
  opendds_export_target_property(${target} "OPENDDS_LANGUAGE_MAPPINGS")

  foreach(idl_file ${_arg_IDL_FILES})
    if (NOT IS_ABSOLUTE ${idl_file})
      set(idl_file ${CMAKE_CURRENT_LIST_DIR}/${idl_file})
    endif()

    get_property(file_language_mappings SOURCE ${idl_file}
      PROPERTY "OPENDDS_LANGUAGE_MAPPINGS")

    get_property(_generated_dependencies SOURCE ${idl_file}
      PROPERTY OPENDDS_IDL_GENERATED_DEPENDENCIES SET)

    if (_generated_dependencies)
      # If an IDL-Generation command was already created this file can safely be
      # skipped; however, the dependencies still need to be added to the target.
      opendds_target_generated_dependencies(${target} ${idl_file} ${_arg_SCOPE})
      list(APPEND language_mappings ${file_language_mappings})

    else()
      list(APPEND non_generated_idl_files ${idl_file})
    endif()
  endforeach()

  if (NOT non_generated_idl_files)
    list(REMOVE_DUPLICATES language_mappings)
    set_property(TARGET ${target}
      PROPERTY "OPENDDS_LANGUAGE_MAPPINGS" ${language_mappings})
    return()
  endif()

  get_property(target_link_libs TARGET ${target} PROPERTY LINK_LIBRARIES)
  if ("OpenDDS::FACE" IN_LIST target_link_libs)
    foreach(_tao_face_flag -SS -Wb,no_fixed_err)
      if (NOT "${_arg_TAO_IDL_FLAGS}" MATCHES "${_tao_face_flag}")
        list(APPEND _arg_TAO_IDL_FLAGS ${_tao_face_flag})
      endif()
    endforeach()

    foreach(_dds_face_flag -GfaceTS -Lface)
      if (NOT "${_arg_DDS_IDL_FLAGS}" MATCHES "${_dds_face_flag}")
        list(APPEND _arg_DDS_IDL_FLAGS ${_dds_face_flag})
      endif()
    endforeach()
  endif()

  file(RELATIVE_PATH working_dir ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})

  if (NOT IS_ABSOLUTE "${working_dir}")
    set(_working_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${working_dir})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/${working_dir})
  else()
    set(_working_binary_dir ${working_dir})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  ## remove trailing slashes
  string(REGEX REPLACE "/$" "" _working_binary_dir ${_working_binary_dir})
  string(REGEX REPLACE "/$" "" _working_source_dir ${_working_source_dir})

  ## opendds_idl would generate different code with the -I flag followed by absolute path
  ## or relative path, if it's a relative path we need to keep it a relative path to the binary tree
  file(RELATIVE_PATH _rel_path_to_source_tree ${_working_binary_dir} ${_working_source_dir})
  if (_rel_path_to_source_tree)
    set(_rel_path_to_source_tree "${_rel_path_to_source_tree}/")
  endif ()

  foreach(flag ${_arg_DDS_IDL_FLAGS})
    if("${flag}" MATCHES "^-I(\\.\\..*)")
      list(APPEND _converted_dds_idl_flags -I${_rel_path_to_source_tree}${CMAKE_MATCH_1})
    else()
      list(APPEND _converted_dds_idl_flags ${flag})
    endif()
  endforeach()

  set(_ddsidl_flags ${_converted_dds_idl_flags})

  foreach(input ${non_generated_idl_files})
    unset(_ddsidl_cmd_arg_-SI)
    unset(_ddsidl_cmd_arg_-GfaceTS)
    unset(_ddsidl_cmd_arg_-o)
    unset(_ddsidl_cmd_arg_-Wb,java)
    unset(_ddsidl_cmd_arg_-Lc++11)
    unset(_ddsidl_cmd_arg_-Lface)
    unset(file_language_mappings)

    cmake_parse_arguments(_ddsidl_cmd_arg
      "-SI;-GfaceTS;-Wb,java;-Lc++11;-Lface"
      "-o" "" ${_ddsidl_flags})

    get_filename_component(noext_name ${input} NAME_WE)
    get_filename_component(abs_filename ${input} ABSOLUTE)
    get_filename_component(file_ext ${input} EXT)
    get_filename_component(idl_file_dir ${abs_filename} DIRECTORY)

    opendds_get_generated_idl_output(
      ${target} ${input} "${_idl_cmd_arg_-o}" output_prefix output_dir)

    if (NOT _ddsidl_cmd_arg_-SI)
      set(_cur_type_support_idl ${output_prefix}TypeSupport.idl)
    else()
      unset(_cur_type_support_idl)
    endif()

    set(_cur_idl_headers ${output_prefix}TypeSupportImpl.h)
    set(_cur_idl_cpp_files ${output_prefix}TypeSupportImpl.cpp)

    if(_ddsidl_cmd_arg_-GfaceTS)
      list(APPEND _cur_idl_headers "${output_prefix}_TS.hpp")
      list(APPEND _cur_idl_cpp_files "${output_prefix}_TS.cpp")
    endif()

    if(_ddsidl_cmd_arg_-Lface)
      list(APPEND _cur_idl_headers "${output_prefix}C.h")
      list(APPEND file_language_mappings "FACE")
    elseif(_ddsidl_cmd_arg_-Lc++11)
      list(APPEND _cur_idl_headers "${output_prefix}C.h")
      list(APPEND file_language_mappings "C++11")
    else()
      set(_cur_idl_file ${input})
      list(APPEND file_language_mappings "C++03")
    endif()

    if (_ddsidl_cmd_arg_-Wb,java)
      set(_cur_java_list "${output_prefix}${file_ext}.TypeSupportImpl.java.list")
      list(APPEND file_dds_idl_flags -j)
      list(APPEND file_language_mappings "Java")
    else()
      unset(_cur_java_list)
    endif()

    set(_cur_idl_outputs ${_cur_idl_headers} ${_cur_idl_cpp_files})

    _tao_append_runtime_lib_dir_to_path(_tao_extra_lib_dirs)

    add_custom_command(
      OUTPUT ${_cur_idl_outputs} ${_cur_type_support_idl} ${_cur_java_list}
      DEPENDS opendds_idl ${DDS_ROOT}/dds/idl/IDLTemplate.txt
      MAIN_DEPENDENCY ${abs_filename}
      COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_INCLUDE_DIR}"
              "${_tao_extra_lib_dirs}"
              $<TARGET_FILE:opendds_idl> -I${idl_file_dir}
              ${_ddsidl_flags} ${file_dds_idl_flags} ${abs_filename}
              -o "${output_dir}"
    )

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_CPP_FILES ${_cur_idl_cpp_files})

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_HEADER_FILES ${_cur_idl_headers})

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_TYPESUPPORT_IDLS ${_cur_type_support_idl})

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_JAVA_OUTPUTS "@${_cur_java_list}")

    set_property(SOURCE ${abs_filename} APPEND PROPERTY
      OPENDDS_LANGUAGE_MAPPINGS ${file_language_mappings})

    if (NOT _arg_SKIP_TAO_IDL)
      tao_idl_command(${target}
        IDL_FLAGS
          -I${DDS_ROOT}
          -I${idl_file_dir} # The type-support IDL will include the primary IDL file
          ${_arg_TAO_IDL_FLAGS}
          -o "${output_dir}"
        IDL_FILES ${_cur_idl_file} ${_cur_type_support_idl})
    endif()

    set_property(SOURCE ${abs_filename} PROPERTY
      OPENDDS_IDL_GENERATED_DEPENDENCIES TRUE)

    opendds_target_generated_dependencies(${target} ${abs_filename} ${_arg_SCOPE})

    list(APPEND language_mappings ${file_language_mappings})
    list(REMOVE_DUPLICATES language_mappings)
  endforeach()

  set_property(TARGET ${target}
    PROPERTY "OPENDDS_LANGUAGE_MAPPINGS" ${language_mappings})
endfunction()
