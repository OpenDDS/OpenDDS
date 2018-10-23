# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

set(TAO_VERSIONING_IDL_FLAGS
  -Wb,versioning_begin=TAO_BEGIN_VERSIONED_NAMESPACE_DECL
  -Wb,versioning_end=TAO_END_VERSIONED_NAMESPACE_DECL
)

if (CORBA_E_MICRO)
  list(APPEND TAO_CORBA_IDL_FLAGS -DCORBA_E_MICRO -Gce)
endif()

if (CORBA_E_COMPACT)
  list(APPEND TAO_CORBA_IDL_FLAGS -DCORBA_E_COMPACT -Gce)
endif()

if (MINIMUM_CORBA)
  list(APPEND TAO_CORBA_IDL_FLAGS -DTAO_HAS_MINIMUM_POA -Gmc)
endif()

if (TAO_NO_IIOP)
  list(APPEND TAO_CORBA_IDL_FLAGS -DTAO_LACKS_IIOP)
endif()

if (GEN_OSTREAM)
  list(APPEND TAO_CORBA_IDL_FLAGS -Gos)
endif()

if (NOT TAO_HAS_OPTIMIZE_COLLOCATED_INVOCATIONS)
  list(APPEND TAO_CORBA_IDL_FLAGS -Sp -Sd)
endif()

function(tao_idl_command name)
  set(multiValueArgs IDL_FLAGS IDL_FILES WORKING_DIRECTORY)
  cmake_parse_arguments(_arg "" "" "${multiValueArgs}" ${ARGN})

  set(_arg_IDL_FLAGS ${_arg_IDL_FLAGS})

  if (NOT _arg_IDL_FILES)
    message(FATAL_ERROR "using tao_idl_command(${name}) without specifying IDL_FILES")
  endif()

  if (NOT _arg_WORKING_DIRECTORY)
    set(_working_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
  elseif (NOT IS_ABSOLUTE "${_arg_WORKING_DIRECTORY}")
    set(_working_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${_arg_WORKING_DIRECTORY})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/${_arg_WORKING_DIRECTORY})
  else()
    set(_working_binary_dir ${_arg_WORKING_DIRECTORY})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  ## convert all include paths to be relative to binary tree instead of to source tree
  file(RELATIVE_PATH _rel_path_to_source_tree ${_working_binary_dir} ${_working_source_dir})
  foreach(flag ${_arg_IDL_FLAGS})
    if ("${flag}" MATCHES "^-I(\\.\\..*)")
       list(APPEND _converted_flags -I${_rel_path_to_source_tree}/${CMAKE_MATCH_1})
     else()
       list(APPEND _converted_flags ${flag})
       # if the flag is like "-Wb,stub_export_file=filename" then set the varilabe
       # "idl_cmd_arg-wb-stub_export_file" to filename
       string(REGEX MATCH "^-Wb,([^=]+)=(.+)" m "${flag}")
       if (m)
         set(idl_cmd_arg-wb-${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
       endif()
    endif()
  endforeach()

  set(optionArgs -Sch -Sci -Scc -Ssh -SS -GA -GT -GX -Gxhst -Gxhsk)
  cmake_parse_arguments(_idl_cmd_arg "${optionArgs}" "-o;-oS;-oA" "" ${_arg_IDL_FLAGS})

  if ("${_idl_cmd_arg_-o}" STREQUAL "")
    set(_output_dir "${_working_binary_dir}")
  else()
    set(_output_dir "${_working_binary_dir}/${_idl_cmd_arg_-o}")
  endif()

  if ("${_idl_cmd_arg_-oS}" STREQUAL "")
    set(_skel_output_dir ${_output_dir})
  else()
    set(_skel_output_dir "${_working_binary_dir}/${_idl_cmd_arg_-oS}")
  endif()

  if ("${_idl_cmd_arg_-oA}" STREQUAL "")
    set(_anyop_output_dir ${_output_dir})
  else()
    set(_anyop_output_dir "${_working_binary_dir}/${_idl_cmd_arg_-oA}")
  endif()

  foreach(idl_file ${_arg_IDL_FILES})

    get_filename_component(idl_file_base ${idl_file} NAME_WE)
    set(_STUB_HEADER_FILES)
    set(_SKEL_HEADER_FILES)

    if (NOT _idl_cmd_arg_-Sch)
      set(_STUB_HEADER_FILES "${_output_dir}/${idl_file_base}C.h")
    endif()

    if (NOT _idl_cmd_arg_-Sci)
      list(APPEND _STUB_HEADER_FILES "${_output_dir}/${idl_file_base}C.inl")
    endif()

    if (NOT _idl_cmd_arg_-Scc)
      set(_STUB_CPP_FILES "${_output_dir}/${idl_file_base}C.cpp")
    endif()

    if (NOT _idl_cmd_arg_-Ssh)
      set(_SKEL_HEADER_FILES "${_skel_output_dir}/${idl_file_base}S.h")
    endif()

    if (NOT _idl_cmd_arg_-SS)
      set(_SKEL_CPP_FILES "${_skel_output_dir}/${idl_file_base}S.cpp")
    endif()

    if (_idl_cmd_arg_-GA)
      set(_ANYOP_HEADER_FILES "${_anyop_output_dir}/${idl_file_base}A.h")
      set(_ANYOP_CPP_FILES "${_anyop_output_dir}/${idl_file_base}A.cpp")
    elseif (_idl_cmd_arg_-GX)
      set(_ANYOP_HEADER_FILES "${_anyop_output_dir}/${idl_file_base}A.h")
    endif()

    if (_idl_cmd_arg_-GT)
      list(APPEND ${idl_file_base}_SKEL_HEADER_FILES
        "${_skel_output_dir}/${idl_file_base}S_T.h"
        "${_skel_output_dir}/${idl_file_base}S_T.cpp")
    endif()

    if (_idl_cmd_arg_-Gxhst)
      list(APPEND _STUB_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/${idl_cmd_arg-wb-stub_export_file})
    endif()

    if (_idl_cmd_arg_-Gxhsk)
      list(APPEND _SKEL_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/${idl_cmd_arg-wb-skel_export_file})
    endif()

    get_filename_component(idl_file_path "${idl_file}" ABSOLUTE)

    set(GPERF_LOCATION $<TARGET_FILE:ace_gperf>)
    if(CMAKE_CONFIGURATION_TYPES)
      get_target_property(is_gperf_imported ace_gperf IMPORTED)
      if (is_gperf_imported)
        set(GPERF_LOCATION $<TARGET_PROPERTY:ace_gperf,LOCATION>)
      endif(is_gperf_imported)
    endif(CMAKE_CONFIGURATION_TYPES)

    if (BUILD_SHARED_LIB AND TARGET TAO_IDL_BE)
      set(tao_idl_shared_libs TAO_IDL_BE TAO_IDL_FE)
    endif()

    set(_OUTPUT_FILES
      ${_STUB_HEADER_FILES}
      ${_SKEL_HEADER_FILES}
      ${_ANYOP_HEADER_FILES}
      ${_STUB_CPP_FILES}
      ${_SKEL_CPP_FILES}
      ${_ANYOP_CPP_FILES})

    add_custom_command(
      OUTPUT ${_OUTPUT_FILES}
      DEPENDS tao_idl ${tao_idl_shared_libs} ace_gperf
      MAIN_DEPENDENCY ${idl_file_path}
      COMMAND ${CMAKE_COMMAND} -E env ${OPENDDS_TAO_IDL_ENV}
        tao_idl -g ${GPERF_LOCATION} ${TAO_CORBA_IDL_FLAGS}
          -Sg -Wb,pre_include=ace/pre.h -Wb,post_include=ace/post.h
          -I${TAO_INCLUDE_DIR} -I${_working_source_dir} ${_converted_flags} ${idl_file_path}
      WORKING_DIRECTORY ${_arg_WORKING_DIRECTORY}
      VERBATIM
    )

    set_property(SOURCE ${idl_file_path} APPEND PROPERTY
      OPENDDS_CPP_FILES
        ${_STUB_CPP_FILES}
        ${_SKEL_CPP_FILES}
        ${_ANYOP_CPP_FILES})

    set_property(SOURCE ${idl_file_path} APPEND PROPERTY
      OPENDDS_HEADER_FILES
        ${_STUB_HEADER_FILES}
        ${_SKEL_HEADER_FILES}
        ${_ANYOP_HEADER_FILES})
  endforeach()
endfunction()
