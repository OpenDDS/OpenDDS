# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/dds_idl_sources.cmake)

function(_opendds_get_sources_and_options
    idl_prefix
    non_idl_prefix
    tao_options
    opendds_options
    suppress_anys
    always_generate_lib_export_header
    skip_tao_idl)
  set(no_value_options
    SKIP_TAO_IDL
  )
  set(single_value_options
    SUPPRESS_ANYS
    ALWAYS_GENERATE_LIB_EXPORT_HEADER
  )
  set(multi_value_options
    PUBLIC PRIVATE INTERFACE
    TAO_IDL_OPTIONS OPENDDS_IDL_OPTIONS
  )
  cmake_parse_arguments(arg "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  macro(handle_files files scope)
    foreach(src ${files})
      get_filename_component(src ${src} ABSOLUTE)

      if("${src}" MATCHES "\\.idl$")
        list(APPEND idl_sources_${scope} ${src})
      else()
        if(non_idl_file_warning)
          message(DEPRECATION "Passing files that aren't IDL files to "
            "opendds_target_sources is deprecated")
          set(non_idl_file_warning OFF)
        endif()

        list(APPEND non_idl_sources_${scope} ${src})
      endif()
    endforeach()
  endmacro()

  # Handle explicit sources per scope
  set(non_idl_file_warning ON)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    set(idl_sources_${scope})
    set(non_idl_sources_${scope})
    handle_files("${arg_${scope}}" ${scope})
  endforeach()

  if(NOT DEFINED arg_SUPPRESS_ANYS)
    set(arg_SUPPRESS_ANYS ${OPENDDS_SUPPRESS_ANYS})
  endif()
  set(${suppress_anys} ${arg_SUPPRESS_ANYS} PARENT_SCOPE)

  if(NOT DEFINED arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER)
    set(arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER ${OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER})
  endif()
  set(${always_generate_lib_export_header} ${arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER} PARENT_SCOPE)

  if(NOT DEFINED arg_SKIP_TAO_IDL)
    set(arg_SKIP_TAO_IDL FALSE)
  endif()
  set(${skip_tao_idl} ${arg_SKIP_TAO_IDL} PARENT_SCOPE)

  handle_files("${arg_UNPARSED_ARGUMENTS}" ${OPENDDS_DEFAULT_SCOPE})

  set(all_idl_files)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    set(idl_sources ${idl_sources_${scope}})
    set(${idl_prefix}_${scope} ${idl_sources} PARENT_SCOPE)
    list(APPEND all_idl_files ${idl_sources})
    set(${non_idl_prefix}_${scope} "${non_idl_sources_${scope}}" PARENT_SCOPE)
  endforeach()

  set(extra_tao_idl_options)
  set(extra_opendds_idl_options)
  if(OPENDDS_FILENAME_ONLY_INCLUDES)
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
      message(WARNING "opendds_target_sources has 2 or more IDL files with \
        the same name. ${check_for_dups_list}")
    endif()

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

function(_opendds_get_target_export_header target export_header_var)
  get_target_property(export_header ${target} OPENDDS_EXPORT_HEADER)
  if(export_header)
    set(${export_header_var} ${export_header} PARENT_SCOPE)
    return()
  endif()

  _opendds_get_generated_file_path(${target} "${target}_export.h" export_header)

  find_file(gen_script "generate_export_file.pl" HINTS ${ACE_BIN_DIR})
  if(NOT EXISTS ${gen_script})
    message(FATAL_ERROR "Failed to find required script 'generate_export_file.pl'")
  endif()

  if(NOT EXISTS ${output_file})
    execute_process(COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_ROOT}"
      ${PERL_EXECUTABLE} ${gen_script} ${target} OUTPUT_FILE ${export_header} RESULT_VARIABLE export_script_exit_status)
    if(NOT export_script_exit_status EQUAL "0")
      message(FATAL_ERROR "Export header script for ${target} exited with ${export_script_status}")
    endif()
  endif()

  set_target_properties(${target}
    PROPERTIES
      OPENDDS_EXPORT_HEADER ${export_header})

  _opendds_add_idl_or_header_files(${target} PUBLIC TRUE ${export_header})

  string(TOUPPER "${target}" target_upper)
  target_compile_definitions(${target}
    PRIVATE
      "${target_upper}_BUILD_DLL")

  set(${export_header_var} ${export_header} PARENT_SCOPE)
endfunction()

function(opendds_target_sources target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Invalid target '${target}' passed into opendds_target_sources")
  endif()

  _opendds_get_sources_and_options(
    idl_sources
    non_idl_sources
    tao_options
    opendds_options
    suppress_anys
    always_generate_lib_export_header
    skip_tao_idl
    ${ARGN})

  if(NOT opendds_options MATCHES "--(no-)?default-nested")
    if(OPENDDS_DEFAULT_NESTED)
      list(APPEND opendds_options "--default-nested")
    else()
      list(APPEND opendds_options "--no-default-nested")
    endif()
  endif()

  get_target_property(target_type ${target} TYPE)
  if(target_type STREQUAL "SHARED_LIBRARY"
      OR (always_generate_lib_export_header AND target_type MATCHES "LIBRARY"))
    _opendds_get_target_export_header(${target} export_header)

    if(NOT "${tao_options}" MATCHES "-Wb,stub_export_include")
      list(APPEND tao_options "-Wb,stub_export_include=${export_header}")
    endif()

    if(NOT "${tao_options}" MATCHES "-Wb,stub_export_macro")
      list(APPEND tao_options "-Wb,stub_export_macro=${target}_Export")
    endif()

    if(NOT "${opendds_options}" MATCHES "-Wb,export_macro")
      list(APPEND opendds_options "-Wb,export_macro=${target}_Export")
    endif()

    if(NOT "${opendds_options}" MATCHES "-Wb,export_include")
      list(APPEND opendds_options "-Wb,export_include=${export_header}")
    endif()
  endif()

  if(NOT "${tao_options}" MATCHES "-SS")
    list(APPEND tao_options "-SS")
  endif()

  list(LENGTH CMAKE_CXX_COMPILER cxx_compiler_length)

  if(${cxx_compiler_length} EQUAL 1)
    if(NOT "${opendds_options}" MATCHES "-Yp")
      list(APPEND opendds_options "-Yp,${CMAKE_CXX_COMPILER}")
    endif()

    if(NOT "${tao_options}" MATCHES "-Yp")
      list(APPEND tao_options "-Yp,${CMAKE_CXX_COMPILER}")
    endif()

  else()
    message(FATAL_ERROR "OpenDDS does not support argument items in CMAKE_CXX_COMPILER.")
  endif()

  if(suppress_anys)
    list(APPEND opendds_options -Sa -St)
    list(APPEND tao_options -Sa -St)
  endif()

  set(includes)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(idl_sources_${scope})
      _opendds_target_idl_sources(${target}
        TAO_IDL_FLAGS ${tao_options}
        DDS_IDL_FLAGS ${opendds_options}
        SKIP_TAO_IDL ${skip_tao_idl}
        IDL_FILES ${idl_sources_${scope}}
        SCOPE ${scope}
        AUTO_INCLUDES auto_includes)
      list(APPEND includes ${auto_includes})
    endif()

    # The above should add IDL-Generated sources; here, the
    # regular c/cpp/h files specified by the user are added.
    target_sources(${target} ${scope} ${non_idl_sources_${scope}})
  endforeach()

  if(idl_sources_PUBLIC OR idl_sources_INTERFACE)
    # Need to include and link internally even if it's just interface files.
    set(max_scope PUBLIC)
  elseif(idl_sources_PRIVATE)
    set(max_scope PRIVATE)
  endif()

  if(OPENDDS_AUTO_LINK_DCPS)
    target_link_libraries(${target} ${max_scope} OpenDDS::Dcps)
  endif()

  _opendds_get_generated_output_dir(${target} generated_directory)
  set_target_properties(${target} PROPERTIES OPENDDS_GENERATED_DIRECTORY ${generated_directory})

  set(is_include_arg FALSE)
  foreach(opendds_idl_arg ${opendds_options})
    if(is_include_arg)
      list(APPEND raw_includes "${opendds_idl_arg}")
      set(is_include_arg FALSE)
    elseif(opendds_idl_arg STREQUAL "-I")
      set(is_include_arg TRUE)
    elseif(opendds_idl_arg MATCHES "-I(.+)")
      list(APPEND raw_includes "${CMAKE_MATCH_1}")
      set(is_include_arg FALSE)
    endif()
  endforeach()

  foreach(raw_include ${raw_includes})
    if(NOT IS_ABSOLUTE ${raw_include})
      set(raw_include "${CMAKE_CURRENT_SOURCE_DIR}/${raw_include}")
    endif()
    list(APPEND includes "${raw_include}")
  endforeach()
  list(REMOVE_DUPLICATES includes)

  if(OPENDDS_USE_CORRECT_INCLUDE_SCOPE)
    set(inc_scope ${max_scope})
  else()
    set(inc_scope PUBLIC)
  endif()
  foreach(include ${includes})
    target_include_directories(${target} ${inc_scope} $<BUILD_INTERFACE:${include}>)
  endforeach()
endfunction()
