# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

macro(_tao_append_runtime_lib_dir_to_path dst)
  if (MSVC)
    set(${dst} "PATH=")
    if (DEFINED ENV{PATH})
      set(${dst} "${${dst}}$ENV{PATH};")
    endif()
    set(${dst} "${${dst}}${TAO_BIN_DIR}")
  else()
    set(${dst} "LD_LIBRARY_PATH=")
    if (DEFINED ENV{LD_LIBRARY_PATH})
      set(${dst} "${${dst}}$ENV{LD_LIBRARY_PATH}:")
    endif()
    set(${dst} "${${dst}}${TAO_LIB_DIR}")
  endif()
endmacro()

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

function(opendds_get_generated_output_dir target output_dir_var)
  # TODO base output_dir_var on target
  set(${output_dir_var} "${CMAKE_CURRENT_BINARY_DIR}/opendds_generated" PARENT_SCOPE)
endfunction()

function(opendds_ensure_generated_output_dir target file o_arg output_dir_var)
  if(o_arg)
    set(output_dir ${o_arg})
  else()
    get_filename_component(abs_file "${file}" ABSOLUTE)
    get_filename_component(abs_dir "${abs_file}" DIRECTORY)
    opendds_get_generated_output_dir("${target}" output_dir)
  endif()
  file(MAKE_DIRECTORY "${output_dir}")
  set(${output_dir_var} "${output_dir}" PARENT_SCOPE)
endfunction()

function(opendds_get_generated_file_path target file output_path_var)
  opendds_ensure_generated_output_dir(${target} ${file} "" output_dir)
  get_filename_component(filename ${file} NAME)
  set(${output_path_var} "${output_dir}/${filename}" PARENT_SCOPE)
endfunction()

function(opendds_get_generated_idl_output target idl_file o_arg output_prefix_var output_dir_var)
  opendds_ensure_generated_output_dir(${target} ${idl_file} "${o_arg}" output_dir)
  get_filename_component(idl_filename_no_ext ${idl_file} NAME_WE)
  set(${output_prefix_var} "${output_dir}/${idl_filename_no_ext}" PARENT_SCOPE)
  set(${output_dir_var} "${output_dir}" PARENT_SCOPE)
endfunction()

function(tao_idl_command target)
  set(multiValueArgs IDL_FLAGS IDL_FILES)
  cmake_parse_arguments(_arg "" "" "${multiValueArgs}" ${ARGN})

  set(_arg_IDL_FLAGS ${_arg_IDL_FLAGS})

  if (NOT _arg_IDL_FILES)
    message(FATAL_ERROR "called tao_idl_command(${target}) without specifying IDL_FILES")
  endif()

  set(_working_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})

  ## convert all include paths to be relative to binary tree instead of to source tree
  file(RELATIVE_PATH _rel_path_to_source_tree ${_working_binary_dir} ${_working_source_dir})
  foreach(flag ${_arg_IDL_FLAGS})
    if("${flag}" MATCHES "^-I(\\.\\..*)")
      list(APPEND _converted_flags -I${_rel_path_to_source_tree}/${CMAKE_MATCH_1})
    else()
      list(APPEND _converted_flags ${flag})
      # if the flag is like "-Wb,stub_export_file=filename" then set the varilabe
      # "idl_cmd_arg-wb-stub_export_file" to filename
      string(REGEX MATCH "^-Wb,([^=]+)=(.+)" m "${flag}")
      if(m)
        set(idl_cmd_arg-wb-${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
      endif()
    endif()
  endforeach()

  set(optionArgs -Sch -Sci -Scc -Ssh -SS -GA -GT -GX -Gxhst -Gxhsk)
  cmake_parse_arguments(_idl_cmd_arg "${optionArgs}" "-o;-oS;-oA" "" ${_arg_IDL_FLAGS})

  foreach(idl_file ${_arg_IDL_FILES})
    set(default_ouput_args)
    opendds_get_generated_idl_output(
      ${target} ${idl_file} "${_idl_cmd_arg_-o}" output_prefix output_dir)
    if(NOT _idl_cmd_arg_-o)
      list(APPEND default_ouput_args "-o" "${output_dir}")
    endif()
    opendds_get_generated_idl_output(
      ${target} ${idl_file} "${_idl_cmd_arg_-oS}" skel_output_prefix skel_output_dir)
    if(NOT _idl_cmd_arg_-oS)
      list(APPEND default_ouput_args "-oS" "${skel_output_dir}")
    endif()
    opendds_get_generated_idl_output(
      ${target} ${idl_file} "${_idl_cmd_arg_-oA}" anyop_output_prefix anyop_output_dir)
    if(NOT _idl_cmd_arg_-oA)
      list(APPEND default_ouput_args "-oA" "${anyop_output_dir}")
    endif()

    set(_STUB_HEADER_FILES)
    set(_SKEL_HEADER_FILES)

    if (NOT _idl_cmd_arg_-Sch)
      set(_STUB_HEADER_FILES "${output_prefix}C.h")
    endif()

    if (NOT _idl_cmd_arg_-Sci)
      list(APPEND _STUB_HEADER_FILES "${output_prefix}C.inl")
    endif()

    if (NOT _idl_cmd_arg_-Scc)
      set(_STUB_CPP_FILES "${output_prefix}C.cpp")
    endif()

    if (NOT _idl_cmd_arg_-Ssh)
      set(_SKEL_HEADER_FILES "${skel_output_prefix}S.h")
    endif()

    if (NOT _idl_cmd_arg_-SS)
      set(_SKEL_CPP_FILES "${skel_output_prefix}S.cpp")
    endif()

    if (_idl_cmd_arg_-GA)
      set(_ANYOP_HEADER_FILES "${anyop_output_prefix}A.h")
      set(_ANYOP_CPP_FILES "${anyop_output_prefix}A.cpp")
    elseif (_idl_cmd_arg_-GX)
      set(_ANYOP_HEADER_FILES "${anyop_output_prefix}A.h")
    endif()

    if (_idl_cmd_arg_-GT)
      list(APPEND _SKEL_HEADER_FILES
        "${skel_output_prefix}S_T.h"
        "${skel_output_prefix}S_T.cpp")
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

    _tao_append_runtime_lib_dir_to_path(_tao_extra_lib_dirs)

    add_custom_command(
      OUTPUT ${_OUTPUT_FILES}
      DEPENDS tao_idl ${tao_idl_shared_libs} ace_gperf
      MAIN_DEPENDENCY ${idl_file_path}
      COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${DDS_ROOT}"  "TAO_ROOT=${TAO_INCLUDE_DIR}"
        "${_tao_extra_lib_dirs}"
        $<TARGET_FILE:tao_idl> -g ${GPERF_LOCATION} ${TAO_CORBA_IDL_FLAGS} -Sg
        -Wb,pre_include=ace/pre.h -Wb,post_include=ace/post.h
        --idl-version 4 -as --unknown-annotations ignore
        -I${TAO_INCLUDE_DIR} -I${_working_source_dir}
        ${_converted_flags}
        ${default_output_args}
        ${idl_file_path}
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
