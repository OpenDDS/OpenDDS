#----------------------------------------------------------------------------
#
# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file provides handling of Type Support files.  It was written
# specifically to work with MPC generated CMakeLists.txt files.  However, it
# is not limited to such uses.
#
#----------------------------------------------------------------------------

include(api_macros)

macro(TYPESUPPORT_FILES_TARGET_SOURCES target)
  set(_multi_value_options PUBLIC PRIVATE INTERFACE TYPESUPPORT_FILES_OPTIONS)
  cmake_parse_arguments(_arg "" "" "${_multi_value_options}" ${ARGN})

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
      OPENDDS_TARGET_SOURCES(${target}
                      ${scope} ${_idl_sources_${scope}}
                      SKIP_TAO_IDL ON
                      OPENDDS_IDL_OPTIONS ${_arg_TYPESUPPORT_FILES_OPTIONS})
   endif()
  endforeach()
endmacro()
