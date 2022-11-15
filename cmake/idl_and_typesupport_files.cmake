#----------------------------------------------------------------------------
#
# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# This file provides handling of IDL and Type Support files.  It was written
# specifically to work with MPC generated CMakeLists.txt files.  However, it
# is not limited to such uses.
#
#----------------------------------------------------------------------------

include(api_macros)

macro(IDL_AND_TYPESUPPORT_FILES_TARGET_SOURCES target)
  set(_multi_value_options PUBLIC PRIVATE INTERFACE
                           IDL_FILES_OPTIONS TYPESUPPORT_FILES_OPTIONS)
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

  ## When building the core libraries, we do not always need (or want) type
  ## support idl to be processed by tao_idl.  In other cases, it is assumed.
  if (BUILDING_OPENDDS_CORE)
    set(SKIP_TYPESUPPORT_IDL ON)
  else()
    set(SKIP_TYPESUPPORT_IDL OFF)
  endif()

  foreach(scope PUBLIC PRIVATE INTERFACE)
    if (_idl_sources_${scope})
      OPENDDS_TARGET_SOURCES(${target}
                      ${scope} ${_idl_sources_${scope}}
                      SKIP_TYPESUPPORT_IDL ${SKIP_TYPESUPPORT_IDL}
                      TAO_IDL_OPTIONS ${_arg_IDL_FILES_OPTIONS}
                      OPENDDS_IDL_OPTIONS ${_arg_TYPESUPPORT_FILES_OPTIONS})
   endif()
  endforeach()
endmacro()
