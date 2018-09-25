# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

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

    elseif("x${arg}" STREQUAL "xSKIP_TAO_IDL")
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
