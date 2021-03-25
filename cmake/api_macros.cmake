# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/dds_idl_sources.cmake)

macro(OPENDDS_GET_SOURCES_AND_OPTIONS
  src_prefix
  idl_prefix
  libs
  tao_options
  opendds_options)

  set(_options_n
    PUBLIC PRIVATE INTERFACE
    TAO_IDL_OPTIONS OPENDDS_IDL_OPTIONS)

  cmake_parse_arguments(_arg "" "" "${_options_n}" ${ARGN})

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

  foreach(arg ${_arg_UNPARSED_ARGUMENTS})
    get_filename_component(arg ${arg} ABSOLUTE)

    if(TARGET ${arg})
      list(APPEND ${libs} ${arg})

    elseif("${arg}" MATCHES "\\.idl$")
      list(APPEND ${idl_prefix}_PRIVATE ${arg})

    else()
      if(${_non_idl_file_warning})
        message(DEPRECATION "Passing files that aren't IDL files to "
          "OPENDDS_TARGET_SOURCES is deprecated")
        set(_non_idl_file_warning OFF)
      endif()

      list(APPEND ${src_prefix}_PRIVATE ${arg})
    endif()
  endforeach()
endmacro()

macro(_OPENDDS_GENERATE_EXPORT_MACRO_COMMAND  target  output)
  set(_bin_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR})
  set(_output_file "${_bin_dir}/${target}_export.h")

  find_file(_gen_script "generate_export_file.pl" HINTS ${ACE_BIN_DIR})
  if(NOT EXISTS ${_gen_script})
    message(FATAL_ERROR "Failed to find required script 'generate_export_file.pl'")
  endif()

  if (NOT EXISTS ${_output_file})
    execute_process(COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_ROOT}"
      perl ${_gen_script} ${target} OUTPUT_FILE ${_output_file} RESULT_VARIABLE _export_script_exit_status)
    if(NOT _export_script_exit_status EQUAL "0")
      message(FATAL_ERROR "Export header script for ${target} exited with ${_export_script_status}")
    endif()
  endif()

  set(${output} ${_output_file})
endmacro()

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
  if(_target_type STREQUAL "SHARED_LIBRARY")

    get_target_property(_export_generated ${target} OPENDDS_EXPORT_GENERATED)
    if(NOT _export_generated)
      _OPENDDS_GENERATE_EXPORT_MACRO_COMMAND(${target} _export_generated)

      set_target_properties(${target}
        PROPERTIES
          OPENDDS_EXPORT_GENERATED ${_export_generated})

      target_sources(${target} PUBLIC ${_export_generated})

      string(TOUPPER "${target}" _target_upper)
      target_compile_definitions(${target}
        PUBLIC
          ${_target_upper}_BUILD_DLL)
    endif()

    if(NOT "${_tao_options}" MATCHES "-Wb,stub_export_include")
      list(APPEND _tao_options "-Wb,stub_export_include=${_export_generated}")
    endif()

    if(NOT "${_tao_options}" MATCHES "-Wb,stub_export_macro")
      list(APPEND _tao_options "-Wb,stub_export_macro=${target}_Export")
    endif()

    if(NOT "${_opendds_options}" MATCHES "-Wb,export_macro")
      list(APPEND _opendds_options "-Wb,export_macro=${target}_Export")
    endif()

    if(NOT "${_opendds_options}" MATCHES "-Wb,export_include")
      list(APPEND _opendds_options "-Wb,export_include=${_export_generated}")
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
endmacro()
