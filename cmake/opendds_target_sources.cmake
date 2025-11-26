# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

if(_OPENDDS_TARGET_SOURCES_CMAKE)
  return()
endif()
set(_OPENDDS_TARGET_SOURCES_CMAKE TRUE)

include("${CMAKE_CURRENT_LIST_DIR}/opendds_group.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/dds_idl_sources.cmake")

# This function handles arguments for opendds_target_sources
function(_opendds_get_sources_and_options
    idl_prefix
    non_idl_prefix
    tao_options
    opendds_options
    use_export
    use_versioned_namespace
    suppress_anys
    export_header_dir
    always_generate_lib_export_header
    generate_server_skeletons
    auto_link
    include_base
    skip_tao_idl
    skip_opendds_idl
    folder)
  set(no_value_options
    SKIP_TAO_IDL
    SKIP_OPENDDS_IDL
  )
  set(single_value_options
    SUPPRESS_ANYS
    EXPORT_HEADER_DIR
    ALWAYS_GENERATE_LIB_EXPORT_HEADER
    GENERATE_SERVER_SKELETONS
    AUTO_LINK
    INCLUDE_BASE
    FOLDER
  )
  set(multi_value_options
    PUBLIC PRIVATE INTERFACE
    TAO_IDL_OPTIONS OPENDDS_IDL_OPTIONS
    USE_EXPORT
    USE_VERSIONED_NAMESPACE
  )
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

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

  # Handle unscoped sources
  handle_files("${arg_UNPARSED_ARGUMENTS}" ${OPENDDS_DEFAULT_SCOPE})

  if(NOT DEFINED arg_SUPPRESS_ANYS)
    set(arg_SUPPRESS_ANYS ${OPENDDS_SUPPRESS_ANYS})
  endif()
  set(${suppress_anys} ${arg_SUPPRESS_ANYS} PARENT_SCOPE)

  if(arg_USE_EXPORT)
    list(LENGTH arg_USE_EXPORT use_export_len)
    if(NOT use_export_len EQUAL 2)
      message(FATAL_ERROR
        "The opendds_target_sources USE_EXPORT option takes a header path and macro name, "
        "but was passed ${use_export_len} values")
    endif()
    set(${use_export} "${arg_USE_EXPORT}" PARENT_SCOPE)
  else()
    if(NOT DEFINED arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER)
      set(arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER ${OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER})
    endif()
    set(${always_generate_lib_export_header} ${arg_ALWAYS_GENERATE_LIB_EXPORT_HEADER} PARENT_SCOPE)
  endif()

  if(DEFINED arg_EXPORT_HEADER_DIR)
    set(${export_header_dir} "${arg_EXPORT_HEADER_DIR}" PARENT_SCOPE)
  else()
    set(${export_header_dir} "" PARENT_SCOPE)
  endif()

  if(arg_USE_VERSIONED_NAMESPACE)
    list(LENGTH arg_USE_VERSIONED_NAMESPACE use_versioned_namespace_len)
    if(NOT use_versioned_namespace_len EQUAL 2)
      message(FATAL_ERROR
        "The opendds_target_sources USE_VERSIONED_NAMESPACE option takes a header path and "
        "macro prefix, but was passed ${use_versioned_namespace_len} values")
    endif()
    set(${use_versioned_namespace} "${arg_USE_VERSIONED_NAMESPACE}" PARENT_SCOPE)
  endif()

  set(${skip_tao_idl} ${arg_SKIP_TAO_IDL} PARENT_SCOPE)
  if(NOT TARGET OpenDDS::opendds_idl)
    set(arg_SKIP_OPENDDS_IDL TRUE)
  endif()
  set(${skip_opendds_idl} ${arg_SKIP_OPENDDS_IDL} PARENT_SCOPE)

  if(NOT DEFINED arg_GENERATE_SERVER_SKELETONS)
    set(arg_GENERATE_SERVER_SKELETONS FALSE)
  endif()
  set(${generate_server_skeletons} ${arg_GENERATE_SERVER_SKELETONS} PARENT_SCOPE)

  if(NOT DEFINED arg_AUTO_LINK)
    set(arg_AUTO_LINK ${OPENDDS_AUTO_LINK_DCPS})
  endif()
  set(${auto_link} ${arg_AUTO_LINK} PARENT_SCOPE)

  if(NOT DEFINED arg_FOLDER)
    set(arg_FOLDER ${OPENDDS_DEFAULT_GENERATED_FOLDER})
  endif()
  set(${folder} ${arg_FOLDER} PARENT_SCOPE)

  set(all_idl_files)
  foreach(scope PUBLIC PRIVATE INTERFACE)
    set(idl_sources ${idl_sources_${scope}})
    set(${idl_prefix}_${scope} ${idl_sources} PARENT_SCOPE)
    list(APPEND all_idl_files ${idl_sources})
    set(${non_idl_prefix}_${scope} "${non_idl_sources_${scope}}" PARENT_SCOPE)
  endforeach()

  if(OPENDDS_FILENAME_ONLY_INCLUDES)
    message(DEPRECATION "OPENDDS_FILENAME_ONLY_INCLUDES is deprecated, use INCLUDE_BASE instead.")
  else()
    if(DEFINED arg_INCLUDE_BASE)
      set("${include_base}" "${arg_INCLUDE_BASE}" PARENT_SCOPE)
    else()
      set(dirs)
      foreach(idl_file ${all_idl_files})
        get_filename_component(idl_file "${idl_file}" REALPATH)
        get_filename_component(dir "${idl_file}" DIRECTORY)
        list(APPEND dirs "${dir}")
      endforeach()
      list(REMOVE_DUPLICATES dirs)
      list(LENGTH dirs dirs_count)
      if(dirs_count GREATER 1)
        message(WARNING "Passing IDL files from different directories, "
          "but INCLUDE_BASE is not being used.")
      endif()
    endif()
  endif()

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

function(opendds_target_sources target)
  set(debug FALSE)
  if(opendds_target_sources IN_LIST OPENDDS_CMAKE_VERBOSE)
    message(STATUS "opendds_target_sources(${target} ${ARGN}) called from ${PROJECT_NAME}")
    set(debug TRUE)
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
  endif()

  if(NOT TARGET ${target})
    message(FATAL_ERROR "Invalid target '${target}' passed into opendds_target_sources")
  endif()

  _opendds_get_sources_and_options(
    idl_sources
    non_idl_sources
    tao_options
    opendds_options
    use_export
    use_versioned_namespace
    suppress_anys
    export_header_dir
    always_generate_lib_export_header
    generate_server_skeletons
    auto_link
    include_base
    skip_tao_idl
    skip_opendds_idl
    folder
    ${ARGN})

  if(NOT opendds_options MATCHES "--(no-)?default-nested")
    if(OPENDDS_DEFAULT_NESTED)
      list(APPEND opendds_options "--default-nested")
    else()
      list(APPEND opendds_options "--no-default-nested")
    endif()
  endif()

  _opendds_get_generated_output_dir(${target} generated_directory MKDIR)
  _opendds_real_path("${generated_directory}" generated_directory)
  set_target_properties(${target} PROPERTIES OPENDDS_GENERATED_DIRECTORY "${generated_directory}")
  set(includes "${generated_directory}")

  if(idl_sources_PUBLIC OR idl_sources_INTERFACE)
    # Need to include and link internally even if it's just interface files.
    set(max_scope PUBLIC)
  elseif(idl_sources_PRIVATE)
    set(max_scope PRIVATE)
  endif()

  get_target_property(target_type ${target} TYPE)
  if(use_export)
    # Assuming the custom export header is compatible with ones generated by generate_export_file.pl
    list(GET use_export 0 existing_header)
    list(GET use_export 1 existing_macro)
    string(FIND ${existing_macro} "_Export" index)
    string(SUBSTRING ${existing_macro} 0 ${index} macro_prefix)
    opendds_export_header(${target} EXISTING INCLUDE ${existing_header} MACRO_PREFIX ${macro_prefix})
  elseif((target_type STREQUAL "SHARED_LIBRARY" AND max_scope STREQUAL "PUBLIC")
      OR (target_type MATCHES "LIBRARY" AND always_generate_lib_export_header))
    opendds_export_header(${target} USE_EXPORT_VAR use_export DIR "${export_header_dir}")
  endif()

  if(use_versioned_namespace)
    list(GET use_versioned_namespace 0 vn_header)
    list(GET use_versioned_namespace 1 vn_prefix)
    set(common_vn_opts
      "-Wb,versioning_include=${vn_header}"
      "-Wb,versioning_begin=${vn_prefix}_BEGIN_VERSIONED_NAMESPACE_DECL"
      "-Wb,versioning_end=${vn_prefix}_END_VERSIONED_NAMESPACE_DECL"
    )
    list(APPEND tao_options ${common_vn_opts})
    list(APPEND opendds_options ${common_vn_opts}
      "-Wb,versioning_name=${vn_prefix}_VERSIONED_NAMESPACE_NAME"
    )
  endif()

  if(NOT "${tao_options}" MATCHES "-SS" AND NOT generate_server_skeletons)
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

  if(DEFINED OPENDDS_CONFIG_INCLUDE_DIR)
    list(APPEND tao_options "-I${OPENDDS_CONFIG_INCLUDE_DIR}")
    list(APPEND opendds_options "-I${OPENDDS_CONFIG_INCLUDE_DIR}")
  endif()

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(idl_sources_${scope})
      set(this_scope_tao_options ${tao_options})
      set(this_scope_opendds_options ${opendds_options})
      if(use_export AND NOT scope STREQUAL "PRIVATE")
        list(GET use_export 0 export_header)
        list(GET use_export 1 export_macro)
        if(NOT "${tao_options}" MATCHES "-Wb,export_include")
          list(APPEND this_scope_tao_options "-Wb,export_include=${export_header}")
        endif()
        if(NOT "${tao_options}" MATCHES "-Wb,export_macro")
          list(APPEND this_scope_tao_options "-Wb,export_macro=${export_macro}")
        endif()
        if(NOT "${opendds_options}" MATCHES "-Wb,export_include")
          list(APPEND this_scope_opendds_options "-Wb,export_include=${export_header}")
        endif()
        if(NOT "${opendds_options}" MATCHES "-Wb,export_macro")
          list(APPEND this_scope_opendds_options "-Wb,export_macro=${export_macro}")
        endif()
      endif()

      _opendds_target_idl_sources(${target}
        TAO_IDL_FLAGS ${this_scope_tao_options}
        DDS_IDL_FLAGS ${this_scope_opendds_options}
        SKIP_TAO_IDL ${skip_tao_idl}
        SKIP_OPENDDS_IDL ${skip_opendds_idl}
        IDL_FILES ${idl_sources_${scope}}
        SCOPE ${scope}
        INCLUDE_BASE "${include_base}"
        AUTO_INCLUDES auto_includes
        USE_EXPORT ${use_export}
        FOLDER ${folder})
      list(APPEND includes ${auto_includes})
    endif()

    # The above should add IDL-Generated sources; here, the
    # regular c/cpp/h files specified by the user are added.
    target_sources(${target} ${scope} ${non_idl_sources_${scope}})
  endforeach()

  if(auto_link)
    if(NOT skip_opendds_idl)
      target_link_libraries(${target} ${max_scope} OpenDDS::Dcps)
    endif()
    target_link_libraries(${target} ${max_scope} TAO::TAO)
  endif()

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
    target_include_directories(${target} ${inc_scope} "$<BUILD_INTERFACE:${include}>")
  endforeach()
endfunction()
