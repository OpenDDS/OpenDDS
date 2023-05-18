# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/dds_idl_sources.cmake)

function(opendds_get_sources_and_options
    idl_prefix
    non_idl_prefix
    tao_options
    opendds_options
    suppress_anys
    always_generate_lib_export_header)

  set(single_value_options
    SUPPRESS_ANYS
    ALWAYS_GENERATE_LIB_EXPORT_HEADER
  )
  set(multi_value_options
    PUBLIC PRIVATE INTERFACE
    TAO_IDL_OPTIONS OPENDDS_IDL_OPTIONS
  )
  cmake_parse_arguments(arg "" "${single_value_options}" "${multi_value_options}" ${ARGN})

  # Handle explicit sources per scope
  set(non_idl_file_warning ON)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    set(idl_sources_${scope})
    set(non_idl_sources_${scope})

    foreach(src ${arg_${scope}})
      get_filename_component(src ${src} ABSOLUTE)

      if("${src}" MATCHES "\\.idl$")
        list(APPEND idl_sources_${scope} ${src})
      else()
        if(${non_idl_file_warning})
          message(DEPRECATION "Passing files that aren't IDL files to "
            "OPENDDS_TARGET_SOURCES is deprecated")
          set(non_idl_file_warning OFF)
        endif()

        list(APPEND non_idl_sources_${scope} ${src})
      endif()
    endforeach()
  endforeach()

  if(DEFINED arg_SUPPRESS_ANYS)
    set(${suppress_anys} ${arg_SUPPRESS_ANYS} PARENT_SCOPE)
  else()
    set(${suppress_anys} ${OPENDDS_SUPPRESS_ANYS} PARENT_SCOPE)
  endif()

  if(DEFINED arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER)
    set(${always_generate_lib_export_header} ${arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER} PARENT_SCOPE)
  else()
    set(${always_generate_lib_export_header} ${OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER} PARENT_SCOPE)
  endif()

  foreach(arg ${arg_UNPARSED_ARGUMENTS})
    get_filename_component(arg ${arg} ABSOLUTE)

    if("${arg}" MATCHES "\\.idl$")
      list(APPEND idl_sources_${OPENDDS_DEFAULT_SCOPE} ${arg})

    else()
      if(${non_idl_file_warning})
        message(DEPRECATION "Passing arguments that aren't IDL files to "
          "OPENDDS_TARGET_SOURCES is deprecated")
        set(non_idl_file_warning OFF)
      endif()

      list(APPEND non_idl_sources_${OPENDDS_DEFAULT_SCOPE} ${arg})
    endif()
  endforeach()

  set(all_idl_files)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    set(idl_sources ${idl_sources_${scope}})
    set(${idl_prefix}_${scope} ${idl_sources} PARENT_SCOPE)
    list(APPEND all_idl_files ${idl_sources})
    set(${non_idl_prefix}_${scope} "${non_idl_sources_${scope}}" PARENT_SCOPE)
  endforeach()

  set(check_for_dups_list "${all_idl_files}")
  list(REMOVE_DUPLICATES check_for_dups_list)
  foreach(fullname ${check_for_dups_list})
    get_filename_component(basename ${fullname} NAME)
    list(APPEND the_idl_files ${basename})
  endforeach()
  list(LENGTH the_idl_files idl_file_count)
  list(REMOVE_DUPLICATES the_idl_files)
  list(LENGTH the_idl_files unique_idl_file_count)
  if(NOT ${idl_file_count} EQUAL ${unique_idl_file_count})
    message(WARNING "OPENDDS_TARGET_SOURCES has 2 or more IDL files with \
      the same name. ${check_for_dups_list}")
  endif()

  set(extra_tao_idl_options)
  set(extra_opendds_idl_options)
  if(OPENDDS_FILENAME_ONLY_INCLUDES)
    set(filename_only_includes)
    foreach(extra ${all_idl_files})
      if("${extra}" MATCHES "\\.idl$")
        get_filename_component(dir ${extra} DIRECTORY)
        if(NOT dir STREQUAL "")
          if(IS_ABSOLUTE "${dir}")
            list(APPEND filename_only_includes "-I${dir}")
          else()
            list(APPEND filename_only_includes "-I${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
          endif()
        endif()
      endif()
    endforeach()
    if(filename_only_includes)
      list(REMOVE_DUPLICATES filename_only_includes)
      set(extra_tao_idl_options "${filename_only_includes}")
      set(extra_opendds_idl_options "${filename_only_includes}" "--filename-only-includes")
    endif()
  endif()

  set(${tao_options} ${arg_TAO_IDL_OPTIONS} ${extra_tao_idl_options} PARENT_SCOPE)
  set(${opendds_options} ${arg_OPENDDS_IDL_OPTIONS} ${extra_opendds_idl_options} PARENT_SCOPE)
endfunction()

function(opendds_get_target_export_header target export_header_var)
  get_target_property(export_header ${target} OPENDDS_EXPORT_HEADER)
  if(export_header)
    set(${export_header_var} ${export_header} PARENT_SCOPE)
    return()
  endif()

  opendds_get_generated_file_path(${target} "${target}_export.h" export_header)

  find_file(gen_script "generate_export_file.pl" HINTS ${ACE_BIN_DIR})
  if(NOT EXISTS ${gen_script})
    message(FATAL_ERROR "Failed to find required script 'generate_export_file.pl'")
  endif()

  if(NOT EXISTS ${output_file})
    execute_process(COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_ROOT}"
      perl ${gen_script} ${target} OUTPUT_FILE ${export_header} RESULT_VARIABLE export_script_exit_status)
    if(NOT export_script_exit_status EQUAL "0")
      message(FATAL_ERROR "Export header script for ${target} exited with ${export_script_status}")
    endif()
  endif()

  set_target_properties(${target}
    PROPERTIES
      OPENDDS_EXPORT_HEADER ${export_header})

  opendds_add_idl_or_header_files(${target} PUBLIC TRUE ${export_header})

  string(TOUPPER "${target}" target_upper)
  target_compile_definitions(${target}
    PRIVATE
      ${target_upper}_BUILD_DLL)

  set(${export_header_var} ${export_header} PARENT_SCOPE)
endfunction()

macro(OPENDDS_TARGET_SOURCES target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Invalid target '${target}' passed into OPENDDS_TARGET_SOURCES")
  endif()

  opendds_get_sources_and_options(
    _idl_sources
    _non_idl_sources
    _tao_options
    _opendds_options
    _suppress_anys
    _always_generate_lib_export_header
    ${ARGN})

  if(NOT _opendds_options MATCHES "--(no-)?default-nested")
    if(OPENDDS_DEFAULT_NESTED)
      list(APPEND _opendds_options "--default-nested")
    else()
      list(APPEND _opendds_options "--no-default-nested")
    endif()
  endif()

  get_target_property(_target_type ${target} TYPE)
  if(_target_type STREQUAL "SHARED_LIBRARY"
      OR (_always_generate_lib_export_header AND _target_type MATCHES "LIBRARY"))
    opendds_get_target_export_header(${target} _export_header)

    if(NOT "${_tao_options}" MATCHES "-Wb,stub_export_include")
      list(APPEND _tao_options "-Wb,stub_export_include=${_export_header}")
    endif()

    if(NOT "${_tao_options}" MATCHES "-Wb,stub_export_macro")
      list(APPEND _tao_options "-Wb,stub_export_macro=${target}_Export")
    endif()

    if(NOT "${_opendds_options}" MATCHES "-Wb,export_macro")
      list(APPEND _opendds_options "-Wb,export_macro=${target}_Export")
    endif()

    if(NOT "${_opendds_options}" MATCHES "-Wb,export_include")
      list(APPEND _opendds_options "-Wb,export_include=${_export_header}")
    endif()
  endif()

  if(NOT "${_tao_options}" MATCHES "-SS")
    list(APPEND _tao_options "-SS")
  endif()

  list(LENGTH CMAKE_CXX_COMPILER cxx_compiler_length)

  if(${cxx_compiler_length} EQUAL 1)
    if(NOT "${_opendds_options}" MATCHES "-Yp")
      list(APPEND _opendds_options "-Yp,${CMAKE_CXX_COMPILER}")
    endif()

    if(NOT "${_tao_options}" MATCHES "-Yp")
      list(APPEND _tao_options "-Yp,${CMAKE_CXX_COMPILER}")
    endif()

  else()
    message(FATAL_ERROR "OpenDDS does not support argument items in CMAKE_CXX_COMPILER.")
  endif()

  if(${_suppress_anys})
    list(APPEND _opendds_options -Sa -St)
    list(APPEND _tao_options -Sa -St)
  endif()

  set(_includes)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(_idl_sources_${scope})
      opendds_target_idl_sources(${target}
        TAO_IDL_FLAGS ${_tao_options} ${OPENDDS_TAO_BASE_IDL_FLAGS}
        DDS_IDL_FLAGS ${_opendds_options} ${OPENDDS_DDS_BASE_IDL_FLAGS} ${_extra_idl_flags}
        IDL_FILES ${_idl_sources_${scope}}
        SCOPE ${scope}
        AUTO_INCLUDES _auto_includes)
      list(APPEND _includes ${_auto_includes})
    endif()

    # The above should add IDL-Generated sources; here, the
    # regular c/cpp/h files specified by the user are added.
    target_sources(${target} ${scope} ${_non_idl_sources_${scope}})
  endforeach()

  if(_idl_sources_PUBLIC OR _idl_sources_INTERFACE)
    # Need to include and link internally even if it's just interface files.
    set(_max_scope PUBLIC)
  elseif(_idl_sources_PRIVATE)
    set(_max_scope PRIVATE)
  endif()

  if(OPENDDS_AUTO_LINK_DCPS)
    target_link_libraries(${target} ${_max_scope} OpenDDS::Dcps)
  endif()

  opendds_get_generated_output_dir(${target} _generated_directory)
  set_target_properties(${target} PROPERTIES OPENDDS_GENERATED_DIRECTORY ${_generated_directory})

  if (DEFINED OPENDDS_RAPIDJSON)
    target_include_directories(${target} PUBLIC "${OPENDDS_ROOT}/tools/rapidjson/include")
  endif()

  set(_is_include_arg FALSE)
  foreach(_opendds_idl_arg ${_opendds_options})
    if(_is_include_arg)
      list(APPEND _raw_includes "${_opendds_idl_arg}")
      set(_is_include_arg FALSE)
    elseif(_opendds_idl_arg STREQUAL "-I")
      set(_is_include_arg TRUE)
    elseif(_opendds_idl_arg MATCHES "-I(.+)")
      list(APPEND _raw_includes "${CMAKE_MATCH_1}")
      set(_is_include_arg FALSE)
    endif()
  endforeach()

  foreach(_raw_include ${_raw_includes})
    if(NOT IS_ABSOLUTE ${_raw_include})
      set(_raw_include "${CMAKE_CURRENT_SOURCE_DIR}/${_raw_include}")
    endif()
    list(APPEND _includes "${_raw_include}")
  endforeach()
  list(REMOVE_DUPLICATES _includes)

  if(OPENDDS_USE_CORRECT_INCLUDE_SCOPE)
    set(_inc_scope ${_max_scope})
  else()
    set(_inc_scope PUBLIC)
  endif()
  foreach(_include ${_includes})
    target_include_directories(${target} ${_inc_scope} $<BUILD_INTERFACE:${_include}>)
  endforeach()
endmacro()
