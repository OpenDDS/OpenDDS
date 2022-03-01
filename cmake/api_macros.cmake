# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/dds_idl_sources.cmake)

macro(OPENDDS_GET_SOURCES_AND_OPTIONS
    src_prefix
    idl_prefix
    libs
    tao_options
    opendds_options
    suppress_anys
    always_generate_lib_export_header)

  set(_single_value_options
    SUPPRESS_ANYS
    ALWAYS_GENERATE_LIB_EXPORT_HEADER
  )
  set(_multi_value_options
    PUBLIC PRIVATE INTERFACE
    TAO_IDL_OPTIONS OPENDDS_IDL_OPTIONS
  )
  cmake_parse_arguments(_arg "" "${_single_value_options}" "${_multi_value_options}" ${ARGN})

  # Handle explicit sources per scope
  set(_non_idl_file_warning ON)
  foreach (scope PUBLIC PRIVATE INTERFACE)
    set(${src_prefix}_${scope})
    set(${idl_prefix}_${scope})

    if(_arg_${scope})
      foreach(src ${_arg_${scope}})
        get_filename_component(src ${src} ABSOLUTE)

        if("${src}" MATCHES "\\.idl$")
          list(APPEND ${idl_prefix}_${scope} ${src})
        else()
          if(${_non_idl_file_warning})
            message(DEPRECATION "Passing files that aren't IDL files to "
              "OPENDDS_TARGET_SOURCES is deprecated")
            set(_non_idl_file_warning OFF)
          endif()

          list(APPEND ${src_prefix}_${scope} ${src})
        endif()
      endforeach()
    endif()
  endforeach()

  set(${tao_options} ${_arg_TAO_IDL_OPTIONS})
  set(${opendds_options} ${_arg_OPENDDS_IDL_OPTIONS})

  if(DEFINED _arg_SUPPRESS_ANYS)
    set(${suppress_anys} ${_arg_SUPPRESS_ANYS})
  else()
    set(${suppress_anys} ${OPENDDS_SUPPRESS_ANYS})
  endif()

  if(DEFINED _arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER)
    set(${always_generate_lib_export_header} ${_arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER})
  else()
    set(${always_generate_lib_export_header} ${OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER})
  endif()

  foreach(arg ${_arg_UNPARSED_ARGUMENTS})
    get_filename_component(arg ${arg} ABSOLUTE)

    if(TARGET ${arg})
      list(APPEND ${libs} ${arg})

    elseif("${arg}" MATCHES "\\.idl$")
      list(APPEND ${idl_prefix}_${OPENDDS_DEFAULT_SCOPE} ${arg})

    else()
      if(${_non_idl_file_warning})
        message(DEPRECATION "Passing files that aren't IDL files to "
          "OPENDDS_TARGET_SOURCES is deprecated")
        set(_non_idl_file_warning OFF)
      endif()

      list(APPEND ${src_prefix}_${OPENDDS_DEFAULT_SCOPE} ${arg})
    endif()
  endforeach()
endmacro()

function(opendds_get_target_export_header target export_header_var)
  get_target_property(export_header ${target} OPENDDS_EXPORT_HEADER)
  if(export_header)
    set(${export_header_var} ${export_header} PARENT_SCOPE)
    return()
  endif()

  opendds_get_generated_file_path(${target} "${target}_export.h" export_header)

  find_file(_gen_script "generate_export_file.pl" HINTS ${ACE_BIN_DIR})
  if(NOT EXISTS ${_gen_script})
    message(FATAL_ERROR "Failed to find required script 'generate_export_file.pl'")
  endif()

  if (NOT EXISTS ${_output_file})
    execute_process(COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_ROOT}"
      perl ${_gen_script} ${target} OUTPUT_FILE ${export_header} RESULT_VARIABLE _export_script_exit_status)
    if(NOT _export_script_exit_status EQUAL "0")
      message(FATAL_ERROR "Export header script for ${target} exited with ${_export_script_status}")
    endif()
  endif()

  set_target_properties(${target}
    PROPERTIES
      OPENDDS_EXPORT_HEADER ${export_header})

  opendds_add_idl_or_header_files(${target} PUBLIC TRUE ${export_header})

  string(TOUPPER "${target}" _target_upper)
  target_compile_definitions(${target}
    PRIVATE
      ${_target_upper}_BUILD_DLL)

  set(${export_header_var} ${export_header} PARENT_SCOPE)
endfunction()

macro(OPENDDS_TARGET_SOURCES target)

  if (NOT TARGET ${target})
    message(FATAL_ERROR "Invalid target '${target}' passed into OPENDDS_TARGET_SOURCES")
  endif()

  set(arglist ${ARGN})

  foreach(the_file ${arglist})
    list(APPEND check_for_dups_list ${the_file})
  endforeach()

  list(REMOVE_DUPLICATES check_for_dups_list)
  unset(the_idl_files)

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

  if(OPENDDS_FILENAME_ONLY_INCLUDES)
    foreach(extra ${arglist})
      if("${extra}" MATCHES "\\.idl$")
        get_filename_component(dir ${extra} DIRECTORY)
        if (NOT dir STREQUAL "")
          list(APPEND _extra_idl_paths "-I${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        endif()
      endif()
    endforeach()
  endif()

  if(_extra_idl_paths)
    list(REMOVE_DUPLICATES _extra_idl_paths)
    list(APPEND _extra_idl_flags "--filename-only-includes")
  endif()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _libs
    _tao_options
    _opendds_options
    _suppress_anys
    _always_generate_lib_export_header
    ${arglist})

  if(NOT _opendds_options MATCHES "--(no-)?default-nested")
    if (OPENDDS_DEFAULT_NESTED)
      list(APPEND _opendds_options "--default-nested")
    else()
      list(APPEND _opendds_options "--no-default-nested")
    endif()
  endif()

  if(_libs)
    message(WARNING "Ignoring libs '${_libs}' passed into OPENDDS_TARGET_SOURCES.")
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

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(_idl_sources_${scope})
      opendds_target_idl_sources(${target}
        TAO_IDL_FLAGS ${_tao_options} ${OPENDDS_TAO_BASE_IDL_FLAGS} ${_extra_idl_paths}
        DDS_IDL_FLAGS ${_opendds_options} ${OPENDDS_DDS_BASE_IDL_FLAGS} ${_extra_idl_paths} ${_extra_idl_flags}
        IDL_FILES ${_idl_sources_${scope}}
        SCOPE ${scope})
    endif()

    # The above should add IDL-Generated sources; here, the
    # regular c/cpp/h files specified by the user are added.
    target_sources(${target} ${scope} ${_sources_${scope}})

  endforeach()

  opendds_get_generated_output_dir(${target} _generated_directory)
  set_target_properties(${target} PROPERTIES OPENDDS_GENERATED_DIRECTORY ${_generated_directory})

  set(_is_include_arg FALSE)
  foreach(_opendds_idl_arg ${_opendds_options})
    if(_is_include_arg)
      list(APPEND _raw_includes "${_opendds_idl_arg}")
      set(_is_include_arg FALSE)
    elseif(_opendds_idl_arg STREQUAL "-I")
      set(_is_include_arg TRUE)
    elseif(_opendds_idl_arg MATCHES "-I(.+)")
      list(APPEND _raw_includes "${CMAKE_MATCH_1}")
    endif()
  endforeach()

  set(_includes "${_generated_directory}")
  foreach(_raw_include ${_raw_includes})
    if(NOT IS_ABSOLUTE ${_raw_include})
      set(_raw_include "${CMAKE_CURRENT_SOURCE_DIR}/${_raw_include}")
    endif()
    list(APPEND _includes "${_raw_include}")
  endforeach()
  list(REMOVE_DUPLICATES _includes)

  foreach(_include ${_includes})
    target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${_include}>)
  endforeach()
endmacro()
