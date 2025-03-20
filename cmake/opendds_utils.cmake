if(_OPENDDS_UTILS_CMAKE)
  return()
endif()
set(_OPENDDS_UTILS_CMAKE TRUE)

function(_opendds_get_generated_output_dir target output_dir_var)
  set(no_value_options MKDIR)
  set(single_value_options O_OPT)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  # TODO base output_dir_var on target
  set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/opendds_generated")
  get_filename_component(output_dir "${output_dir}" REALPATH)
  if(arg_O_OPT)
    if(IS_ABSOLUTE "${arg_O_OPT}")
      set(output_dir "${arg_O_OPT}")
    else()
      set(output_dir "${output_dir}/${arg_O_OPT}")
    endif()
  endif()

  if(arg_MKDIR)
    file(MAKE_DIRECTORY "${output_dir}")
  endif()

  set(${output_dir_var} "${output_dir}" PARENT_SCOPE)
endfunction()

function(_opendds_get_generated_output target file)
  set(no_value_options MKDIR)
  set(single_value_options
    INCLUDE_BASE
    O_OPT
    DIR_PATH_VAR
    FILE_PATH_VAR
    PREFIX_PATH_VAR
    FAIL_VAR
    EXPECT_FAIL
  )
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  if(arg_FAIL_VAR)
    set(${arg_FAIL_VAR} FALSE PARENT_SCOPE)
  endif()

  set(dir_args)
  if(arg_MKDIR)
    list(APPEND dir_args MKDIR)
  endif()
  if(arg_O_OPT)
    list(APPEND dir_args O_OPT "${arg_O_OPT}")
  endif()
  _opendds_get_generated_output_dir("${target}" output_dir ${dir_args})

  if(arg_INCLUDE_BASE AND NOT OPENDDS_FILENAME_ONLY_INCLUDES AND NOT arg_O_OPT)
    # We need to recreate the directory structure of the source IDL with the
    # include base as the root.
    get_filename_component(real_include_base "${arg_INCLUDE_BASE}" REALPATH)
    if(IS_ABSOLUTE "${file}")
      get_filename_component(real_file "${file}" REALPATH)
    else()
      get_filename_component(real_file "${CMAKE_CURRENT_SOURCE_DIR}/${file}" REALPATH)
    endif()
    file(RELATIVE_PATH rel_file "${real_include_base}" "${real_file}")
    if(rel_file MATCHES "^\\.\\.")
      set(msg_type FATAL_ERROR)
      if(arg_FAIL_VAR)
        set(${arg_FAIL_VAR} TRUE PARENT_SCOPE)
        set(msg_type SEND_ERROR)
      endif()
      if(arg_EXPECT_FAIL)
        return()
      endif()
      message(${msg_type}
        "  This file:\n"
        "  \n"
        "    ${rel_file}\n"
        "    (${file})\n"
        "  \n"
        "  is outside the INCLUDE_BASE:\n"
        "  \n"
        "    ${arg_INCLUDE_BASE}\n"
        "    (${real_include_base})\n")
      return()
    endif()
    get_filename_component(rel_dir "${rel_file}" DIRECTORY)
    set(output_dir "${output_dir}/${rel_dir}")
  endif()

  string(REGEX REPLACE "/$" "" output_dir "${output_dir}")

  if(arg_MKDIR)
    file(MAKE_DIRECTORY "${output_dir}")
  endif()

  if(arg_DIR_PATH_VAR)
    set(${arg_DIR_PATH_VAR} "${output_dir}" PARENT_SCOPE)
  endif()

  if(arg_FILE_PATH_VAR)
    get_filename_component(filename "${file}" NAME)
    set(${arg_FILE_PATH_VAR} "${output_dir}/${filename}" PARENT_SCOPE)
  endif()

  if(arg_PREFIX_PATH_VAR)
    get_filename_component(filename_no_ext "${file}" NAME_WE)
    set(${arg_PREFIX_PATH_VAR} "${output_dir}/${filename_no_ext}" PARENT_SCOPE)
  endif()
endfunction()

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

function(opendds_export_header target)
  set(no_value_options EXISTING)
  set(single_value_options
    DIR
    USE_EXPORT_VAR
    INCLUDE
    MACRO_PREFIX
    EXPORT_MACRO
    HEADER_MACRO
    SOURCE_MACRO
    FORCE_EXPORT
  )
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  get_target_property(use_export ${target} OPENDDS_USE_EXPORT)
  if(use_export)
    # Export header has already been set.
    if(DEFINED arg_USE_EXPORT_VAR)
      set(${arg_USE_EXPORT_VAR} "${use_export}" PARENT_SCOPE)
    endif()
    return()
  endif()

  if(NOT DEFINED arg_INCLUDE)
    set(arg_INCLUDE "${target}_export.h")
    if(arg_DIR)
      set(arg_INCLUDE "${arg_DIR}/${arg_INCLUDE}")
    endif()
  endif()

  if(NOT DEFINED arg_MACRO_PREFIX)
    set(arg_MACRO_PREFIX ${target})
  endif()
  string(TOUPPER "${arg_MACRO_PREFIX}" uppercase_macro_prefix)

  if(NOT DEFINED arg_EXPORT_MACRO)
    set(arg_EXPORT_MACRO "${arg_MACRO_PREFIX}_Export")
  endif()

  if(NOT DEFINED arg_HEADER_MACRO)
    set(arg_HEADER_MACRO "${uppercase_macro_prefix}_HAS_DLL")
  endif()

  if(NOT DEFINED arg_SOURCE_MACRO)
    set(arg_SOURCE_MACRO "${uppercase_macro_prefix}_BUILD_DLL")
  endif()

  if(DEFINED arg_FORCE_EXPORT)
    if(arg_FORCE_EXPORT)
      set(export 1)
    else()
      set(export 0)
    endif()
  else()
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "SHARED_LIBRARY")
      set(export 1)
    elseif(target_type STREQUAL "STATIC_LIBRARY")
      set(export 0)
    else()
      message(FATAL_ERROR "Target ${target} has unexpected type ${target_type}")
    endif()
  endif()

  # Set macro for library source files. This is the same thing as dynamicflags
  # in MPC.
  if(export)
    target_compile_definitions(${target} PRIVATE "${arg_SOURCE_MACRO}")
    set_target_properties(${target} PROPERTIES CXX_VISIBILITY_PRESET hidden)
    if(APPLE)
      # Override ACE's config-macosx-mavericks.h
      target_compile_definitions(${target} PRIVATE "ACE_HAS_CUSTOM_EXPORT_MACROS=1")
    endif()
  endif()

  # Set macro for library header files that determines if symbols are exported.
  # It doesn't have to be explicitly set in MPC because usually static and
  # shared libraries are not mixed. CMake easily allows this though, so we have
  # to explicitly tell the export headers files if symbols should be exported
  # because otherwise it will default based on ACE_AS_STATIC_LIBS.
  #
  # This macro is also used by dds/DCPS/InitStaticLibs.h in user targets using
  # OpenDDS to include static initialization headers when needed.
  target_compile_definitions(${target} PUBLIC "${arg_HEADER_MACRO}=${export}")

  if(NOT arg_EXISTING)
    # Figure out the generated header path
    _opendds_get_generated_output_dir(${target} gendir MKDIR)
    _opendds_get_generated_output(${target} "${gendir}/${arg_INCLUDE}"
      INCLUDE_BASE "${gendir}" MKDIR FILE_PATH_VAR export_header_path)

    # Then generate it
    set(export_macro "${arg_EXPORT_MACRO}")
    set(header_macro "${arg_HEADER_MACRO}")
    set(source_macro "${arg_SOURCE_MACRO}")
    configure_file("${_OPENDDS_CMAKE_DIR}/export.h.in" "${export_header_path}")
    _opendds_add_idl_or_header_files(${target} PUBLIC TRUE "${export_header_path}")
  endif()

  set(use_export "${arg_INCLUDE};${arg_EXPORT_MACRO}")
  set_target_properties(${target} PROPERTIES OPENDDS_USE_EXPORT "${use_export}")
  if(DEFINED arg_USE_EXPORT_VAR)
    set(${arg_USE_EXPORT_VAR} "${use_export}" PARENT_SCOPE)
  endif()
endfunction()

macro(_opendds_save_cache name type value)
  list(APPEND _opendds_save_cache_vars ${name})
  set(_opendds_save_cache_${name}_type ${type})
  set(_opendds_save_cache_${name}_value "${${name}}")
  set(${name} "${value}" CACHE ${type} "" FORCE)
endmacro()

macro(_opendds_restore_cache)
  foreach(name ${_opendds_save_cache_vars})
    set(${name} "${_opendds_save_cache_${name}_value}" CACHE
      "${_opendds_save_cache_${name}_type}" "" FORCE)
    unset(_opendds_save_cache_${name}_type)
    unset(_opendds_save_cache_${name}_value)
  endforeach()
  unset(_opendds_save_cache_vars)
endmacro()

function(_opendds_pop_list list_var)
  set(list "${${list_var}}")
  list(LENGTH list len)
  if(len GREATER 0)
    math(EXPR last "${len} - 1")
    list(REMOVE_AT list "${last}")
    set("${list_var}" "${list}" PARENT_SCOPE)
  endif()
endfunction()

function(_opendds_path_list path_list_var)
  if("APPEND" IN_LIST ARGN)
    set(path_list "${${path_list_var}}")
    list(REMOVE_ITEM ARGN APPEND)
  else()
    set(path_list)
  endif()

  if(WIN32)
    set(delimiter ";")
  else()
    set(delimiter ":")
  endif()

  foreach(path ${ARGN})
    if(path_list AND NOT path_list MATCHES "${delimiter}$")
      set(path_list "${path_list}${delimiter}")
    endif()
    set(path_list "${path_list}${path}")
  endforeach()

  set("${path_list_var}" "${path_list}" PARENT_SCOPE)
endfunction()

function(opendds_msvc_bigobj target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()
