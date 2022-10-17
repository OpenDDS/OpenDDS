#----------------------------------------------------------------------------
#
# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file provides handling of IDL files.  It was written specifically to
# work with MPC generated CMakeLists.txt files.  However, it is not limited
# to such uses.
#
#----------------------------------------------------------------------------

include(tao_idl_sources)

macro(IDL_FILES_TARGET_SOURCES target)
  set(_multi_value_options PUBLIC PRIVATE INTERFACE IDL_FILES_OPTIONS)
  cmake_parse_arguments(_arg "" "" "${_multi_value_options}" ${ARGN})
  cmake_parse_arguments(_idl_cmd_arg "-o" "" ${_arg_IDL_FILES_OPTIONS})

  foreach(scope PUBLIC PRIVATE INTERFACE)
    set(_idl_sources_${scope})

    if(_arg_${scope})
      foreach(src ${_arg_${scope}})
        get_filename_component(src ${src} ABSOLUTE)

        if("${src}" MATCHES "\\.idl$")
          list(APPEND _idl_sources_${scope} ${src})
        endif()
      endforeach()
    endif()
  endforeach()

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if (_idl_sources_${scope})
      foreach(file ${_idl_sources_${scope}})
        get_source_file_property(cpps ${file} OPENDDS_CPP_FILES)
        if (NOT cpps)
          opendds_get_generated_idl_output(
            ${target} ${file} "${_idl_cmd_arg_-o}" output_prefix output_dir)

          tao_idl_command(${target}
                          IDL_FLAGS ${_arg_IDL_FILES_OPTIONS} -o ${output_dir}
                          IDL_FILES ${file})
          get_source_file_property(cpps ${file} OPENDDS_CPP_FILES)
          target_sources(${target} ${scope} ${cpps})
        endif()
      endforeach()
    endif()
  endforeach()
endmacro()
