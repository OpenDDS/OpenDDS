# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

foreach(_dependency tao_idl_sources dds_idl_sources)
  include(${CMAKE_CURRENT_LIST_DIR}/${_dependency}.cmake)
endforeach()

function(opendds_include_dirs_once)
  get_directory_property(includes INCLUDE_DIRECTORIES)

  foreach (i ${OPENDDS_INCLUDE_DIRS})
    if (NOT "${i}" IN_LIST includes)
      include_directories(${i})
    endif()
  endforeach()
endfunction()

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
  foreach (scope PUBLIC PRIVATE INTERFACE)
    set(${src_prefix}_${scope})
    set(${idl_prefix}_${scope})

    if(_arg_${scope})
      foreach(src ${_arg_${scope}})
        if("${src}" MATCHES "\\.idl$")
          list(APPEND ${idl_prefix}_${scope} ${src})
        else()
          list(APPEND ${src_prefix}_${scope} ${src})
        endif()
      endforeach()
    endif()
  endforeach()

  set(${tao_options} ${_arg_TAO_IDL_OPTIONS})
  set(${opendds_options} ${_arg_OPENDDS_IDL_OPTIONS})

  foreach(arg ${_arg_UNPARSED_ARGUMENTS})
    if(TARGET ${arg})
      list(APPEND ${libs} ${arg})

    elseif("${arg}" MATCHES "\\.idl$")
      # Implicit sources default to PUBLIC
      list(APPEND ${idl_prefix}_PUBLIC ${arg})

    else()
      list(APPEND ${src_prefix}_PUBLIC ${arg})
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

  add_custom_command(
    OUTPUT ${_output_file}
    DEPENDS perl
    COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}" "TAO_ROOT=${TAO_ROOT}"
      $<TARGET_FILE:perl> ${_gen_script} ${target} $<ANGLE-R> ${_output_file}
    VERBATIM
  )

  set(${output} ${_output_file})
endmacro()

# OPENDDS_TARGET_SOURCES(target
#   [items...]
#   [<INTERFACE|PUBLIC|PRIVATE> items...])
#
# This macro behaves similarly to target_sources(...) with the following
# differences:
#   1) Items can be either C/C++ sources or IDL sources.
#   2) The scope-qualifier (PUBLIC, PRIVATE, INTERFACE) is not required.
#      When it is omitted, PUBLIC is used by default.
#
# When IDL sources are supplied, custom commands are generated which will
# be invoked to compile the IDL sources into their component cpp/h files.
#
# A custom command will also be added to generate the required IDL export
# header file (*target*_export.h) to add the required export macros. This
# file is then added as a dependency for the supplied target.
#
# The following defines are implicitly added to the target and any exe/lib
# which uses the library:
#   *TARGET*_BUILD_DLL
#   OPENDDS_SECURITY (if security is enabled)
#
macro(OPENDDS_TARGET_SOURCES target)

  if (NOT TARGET ${target})
    message(FATAL_ERROR "Invalid target '${target}' passed into OPENDDS_TARGET_SOURCES")
  endif()

  opendds_include_dirs_once()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _libs
    _tao_options
    _opendds_options
    ${ARGN})

  if(_libs)
    message(WARNING "Ignoring libs '${_libs}' passed into OPENDDS_TARGET_SOURCES.")
  endif()

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
        ${_target_upper}_BUILD_DLL
        ${OPENDDS_DCPS_COMPILE_DEFS})

    if (OPENDDS_DCPS_LINK_DEPS)
      target_link_libraries(${target} ${OPENDDS_DCPS_LINK_DEPS})
    endif()

  endif()

  if(NOT "${_tao_options}" MATCHES "-Wb,stub_export_include")
    list(APPEND _tao_options "-Wb,stub_export_include=${_export_generated}")
  endif()

  if(NOT "${_tao_options}" MATCHES "-Wb,stub_export_macro")
    list(APPEND _tao_options "-Wb,stub_export_macro=${target}_Export")
  endif()

  if(NOT "${_tao_options}" MATCHES "-SS")
    list(APPEND _tao_options "-SS")
  endif()

  if(NOT "${_opendds_options}" MATCHES "-Wb,export_macro")
    list(APPEND _opendds_options "-Wb,export_macro=${target}_Export")
  endif()

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(_idl_sources_${scope})
      opendds_target_idl_sources(${target}
        TAO_IDL_FLAGS ${_tao_options} ${OPENDDS_TAO_BASE_IDL_FLAGS}
        DDS_IDL_FLAGS ${_opendds_options} ${OPENDDS_DDS_BASE_IDL_FLAGS}
        IDL_FILES ${_idl_sources_${scope}})
    endif()

    # The above should add IDL-Generated sources; here, the
    # regular c/cpp/h files specified by the user are added.
    target_sources(${target} ${scope} ${_sources_${scope}})

  endforeach()
endmacro()
