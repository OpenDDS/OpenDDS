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
  _sources _idl_sources
  _cmake_options _tao_options _opendds_options)

  set(${_sources})
  set(${_idl_sources})
  set(${_cmake_options})
  set(${_tao_options})
  set(${_opendds_options})
  set(_found_tao_options FALSE)
  set(_found_opendds_options FALSE)

  foreach(arg ${ARGN})
    if("x${arg}" STREQUAL "xTAO_IDL_OPTIONS")
      set(_found_tao_options TRUE)
      set(_found_opendds_options FALSE)

    elseif("x${arg}" STREQUAL "xOPENDDS_IDL_OPTIONS")
      set(_found_tao_options FALSE)
      set(_found_opendds_options TRUE)

    elseif(
        "x${arg}" STREQUAL "xWIN32" OR
        "x${arg}" STREQUAL "xMACOSX_BUNDLE" OR
        "x${arg}" STREQUAL "xEXCLUDE_FROM_ALL" OR
        "x${arg}" STREQUAL "xSTATIC" OR
        "x${arg}" STREQUAL "xSHARED" OR
        "x${arg}" STREQUAL "xMODULE" )
      list(APPEND ${_cmake_options} ${arg})

    else()
      if(_found_tao_options)
        list(APPEND ${_tao_options} ${arg})

      elseif (_found_opendds_options)
        list(APPEND ${_opendds_options} ${arg})

      else()
        if(${arg} MATCHES "\\.idl$")
          list(APPEND ${_idl_sources} ${arg})
        else()
          list(APPEND ${_sources} ${arg})
        endif()
      endif()
    endif()
  endforeach()
endmacro()

# OPENDDS_IDL_COMMANDS(target format generated_files
#                tao_options
#                opendds_options
#                file0 file1 ...
#                [STATIC | SHARED | MODULE])
macro(OPENDDS_IDL_COMMANDS _target format generated_files)
  set(_argn_list "${ARGN}")
endmacro()

# OPENDDS_ADD_LIBRARY(target file0 file1 ...
#                 [STATIC | SHARED | MODULE] [EXCLUDE_FROM_ALL]
#                 [TAO_IDL_OPTIONS ...]
#                 [OPENDDS_IDL_OPTIONS ...])
macro(OPENDDS_ADD_LIBRARY _target)

  OPENDDS_INCLUDE_DIRS_ONCE()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _cmake_options
    _tao_options
    _opendds_options
    ${ARGN})

  OPENDDS_IDL_COMMANDS(${_target} OBJ _generated_files
    ${_tao_options} ${_opendds_options}
    ${_idl_sources}
    ${_cmake_options})

  add_library(${_target} ${_cmake_options}
    ${_generated_files}
    ${_sources})

endmacro()

# OPENDDS_ADD_EXECUTABLE(target file0 file1 ...
#                    [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL]
#                    [TAO_IDL_OPTIONS ...]
#                    [OPENDDS_IDL_OPTIONS ...] )
macro(OPENDDS_ADD_EXECUTABLE _target)

  OPENDDS_INCLUDE_DIRS_ONCE()

  OPENDDS_GET_SOURCES_AND_OPTIONS(
    _sources
    _idl_sources
    _cmake_options
    _tao_options
    _opendds_options
    ${ARGN})

  OPENDDS_IDL_COMMANDS(${_target} OBJ _generated_files
    ${_tao_options} ${_opendds_options}
    ${_idl_sources}
    ${_cmake_options})

  add_executable(${_target} ${_cmake_options}
    ${_generated_files}
    ${_sources})

endmacro()
