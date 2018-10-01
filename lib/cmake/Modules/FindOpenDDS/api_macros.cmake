# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

foreach(_dependency  ace_utils tao_idl_sources dds_idl_sources)
  include(${CMAKE_CURRENT_LIST_DIR}/${_dependency}.cmake)
endforeach()

function(OPENDDS_INCLUDE_DIRS_ONCE)
  get_directory_property(_include_directories INCLUDE_DIRECTORIES)
  set(_add TRUE)
  if(_include_directories)
    foreach(dir ${_include_directories})
      if("${dir}" STREQUAL "${OPENDDS_INCLUDE_DIRS}")
        set(_add FALSE)
      endif()
    endforeach()
  endif()
  if(_add)
    include_directories(${OPENDDS_INCLUDE_DIRS})
  endif()
endfunction()


macro(OPENDDS_GET_SOURCES_AND_OPTIONS
  src_prefix
  idl_prefix
  libs
  cmake_options
  tao_options
  opendds_options
  options)

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

  set(${cmake_options})
  set(${options})

  foreach(arg ${_arg_UNPARSED_ARGUMENTS})
    if("x${arg}" STREQUAL "xWIN32" OR
       "x${arg}" STREQUAL "xMACOSX_BUNDLE" OR
       "x${arg}" STREQUAL "xEXCLUDE_FROM_ALL" OR
       "x${arg}" STREQUAL "xSTATIC" OR
       "x${arg}" STREQUAL "xSHARED" OR
       "x${arg}" STREQUAL "xMODULE")
      list(APPEND ${cmake_options} ${arg})

    elseif("x${arg}" STREQUAL "xSKIP_TAO_IDL" OR
           "x${arg}" STREQUAL "xSKIP_TAO_IDL_EXPORT")
      list(APPEND ${options} ${arg})

    else()
      if(TARGET ${arg})
        list(APPEND ${libs} ${arg})

      elseif("${arg}" MATCHES "\\.idl$")
        # Implicit sources default to PUBLIC
        list(APPEND ${idl_prefix}_PUBLIC ${arg})

      else()
        list(APPEND ${src_prefix}_PUBLIC ${arg})
      endif()
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

macro(OPENDDS_TARGET_SOURCES target)
  OPENDDS_INCLUDE_DIRS_ONCE()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _libs
    _cmake_options
    _tao_options
    _opendds_options
    _options
    ${ARGN})

  if(_libs)
    message(WARNING "Ignoring libs '${_libs}' passed into OPENDDS_TARGET_SOURCES.")
  endif()

  get_property(_export_generated TARGET ${target}
    PROPERTY OPENDDS_EXPORT_GENERATED SET)

  if(NOT _export_generated)
    _OPENDDS_GENERATE_EXPORT_MACRO_COMMAND(${target} _export_generated)

    set_property(TARGET ${target}
      PROPERTY OPENDDS_EXPORT_GENERATED ${_export_generated})

    target_sources(${target} PUBLIC ${_export_generated})

    string(TOUPPER "${target}" _target_upper)
    target_compile_definitions(${target} PUBLIC ${_target_upper}_BUILD_DLL)
  endif()

  if(NOT "${tao_options}" MATCHES "-Wb,stub_export_include")
    list(APPEND tao_options "-Wb,stub_export_include=${_export_generated}")
  endif()

  if(NOT "${tao_options}" MATCHES "-Wb,stub_export_macro")
    list(APPEND tao_options "-Wb,stub_export_macro=${target}_Export")
  endif()

  if(NOT "${tao_options}" MATCHES "-SS")
    list(APPEND tao_options "-SS")
  endif()

  if(NOT "${_opendds_options}" MATCHES "-Wb,export_macro")
    list(APPEND opendds_options "-Wb,export_macro=${target}_Export")
  endif()

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(_idl_sources_${scope})
      dds_idl_sources(
        TARGETS ${target}
        TAO_IDL_FLAGS ${tao_options}
        DDS_IDL_FLAGS ${opendds_options}
        IDL_FILES ${_idl_sources_${scope}}
        ${options})
    endif()

    # The above should add IDL-Generated sources; here, the
    # regular c/cpp/h files specified by the user are added.
    target_sources(${target} ${scope} ${_sources_${scope}})

  endforeach()
endmacro()


macro(OPENDDS_IDL_COMMANDS target
  idl_prefix
  src_prefix
  cmake_options
  tao_options
  opendds_options
  options)

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if(${idl_prefix}_${scope})
      dds_idl_sources(
        TARGETS ${target}
        TAO_IDL_FLAGS ${tao_options}
        DDS_IDL_FLAGS ${opendds_options}
        IDL_FILES ${${idl_prefix}_${scope}}
        ${options})
    endif()
  endforeach()
endmacro()


# OPENDDS_ADD_LIBRARY(target
#   file0 file1 ...
#   [<INTERFACE|PUBLIC|PRIVATE> file0 file1...] ...]
#   [lib0 lib1 ...]
#   [STATIC | SHARED | MODULE] [EXCLUDE_FROM_ALL]
#   [SKIP_TAO_IDL]
#   [TAO_IDL_OPTIONS ...]
#   [OPENDDS_IDL_OPTIONS ...])
macro(OPENDDS_ADD_LIBRARY target)

  OPENDDS_INCLUDE_DIRS_ONCE()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _libs
    _cmake_options
    _tao_options
    _opendds_options
    _options
    ${ARGN})

  add_library(${target} ${_cmake_options})
  target_link_libraries(${target} ${_libs})

  OPENDDS_IDL_COMMANDS(${target}
    _idl_sources
    _sources
    "${_cmake_options}"
    "${_tao_options}"
    "${_opendds_options}"
    "${_options}")

  foreach(scope PUBLIC PRIVATE INTERFACE)
    target_sources(${target} ${scope} ${_sources_${scope}})
  endforeach()
endmacro()


# OPENDDS_ADD_EXECUTABLE(target
#   file0 file1 ...
#   [<INTERFACE|PUBLIC|PRIVATE> file0 file1...] ...]
#   [lib0 lib1 ...]
#   [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL]
#   [SKIP_TAO_IDL]
#   [TAO_IDL_OPTIONS ...]
#   [OPENDDS_IDL_OPTIONS ...])
macro(OPENDDS_ADD_EXECUTABLE target)

  OPENDDS_INCLUDE_DIRS_ONCE()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _libs
    _cmake_options
    _tao_options
    _opendds_options
    _options
    ${ARGN})

  add_executable(${target} ${_cmake_options})
  target_link_libraries(${target} ${_libs})

  OPENDDS_IDL_COMMANDS(${target}
    _idl_sources
    _sources
    "${_cmake_options}"
    "${_tao_options}"
    "${_opendds_options}"
    "${_options}")

  foreach(scope PUBLIC PRIVATE INTERFACE)
    target_sources(${target} ${scope} ${_sources_${scope}})
  endforeach()
endmacro()
