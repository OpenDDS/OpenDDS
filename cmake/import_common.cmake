# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# Common system for the OpenDDS*Config.cmake files to declare imported
# libraries and executables.
#
# The system works like this:
# - Declare a list of all libraries to find. This is the library group. This
#   must use the filename minus any "lib" prefix and file extensions.
# - Find any programs needed manually using find_program.
# - For each library in the group list, a list can be defined to add
#   properties. They must begin with a prefix that is the upper case version of
#   the name in the group list.
#   - <prefix>_DEPS: a required list of required libraries and programs.
#     Programs names must be the name from find_program. Libraries must be the
#     name the user would use (<group>::<name>).
#   - <prefix>_INCLUDE_DIRS
#   - <prefix>_COMPILE_DEFINITIONS
#   - <prefix>_COMPILE_OPTIONS
# - Call _opendds_find_our_libraries with the group name and group lib list.
# - Call _opendds_found_required_deps with the FOUND variable name and a list
#   of required dependencies. The list should use the format
#   "SelectLibraryConfigurations" makes.
# - If found call _opendds_add_target_binary to declare imported execuatables
#   and _opendds_add_library_group to declare the imported libraries.

if(_OPENDDS_IMPORT_COMMON_CMAKE)
  return()
endif()
set(_OPENDDS_IMPORT_COMMON_CMAKE TRUE)

cmake_minimum_required(VERSION 3.3.2)

include(SelectLibraryConfigurations)

include("${CMAKE_CURRENT_LIST_DIR}/init.cmake")
if(NOT DEFINED ACE_ROOT OR NOT DEFINED TAO_ROOT)
  return()
endif()

function(_opendds_find_our_libraries_for_config lib_group_name libs config suffix)
  if(MSVC AND OPENDDS_STATIC)
    set(suffix "s${suffix}")
  endif()

  set(lib_dir "${${lib_group_name}_LIB_DIR}")

  foreach(lib ${libs})
    set(lib_file_base "${lib}${suffix}")
    string(TOUPPER ${lib} var_prefix)
    set(lib_var "${var_prefix}_LIBRARY_${config}")

    find_library(${lib_var} "${lib_file_base}" HINTS "${lib_dir}")
    set(found_var "${var_prefix}_LIBRARY_FOUND")
    if(${lib_var})
      set(${found_var} TRUE PARENT_SCOPE)

      # Workaround https://gitlab.kitware.com/cmake/cmake/-/issues/23249
      # These paths might be symlinks and IMPORTED_RUNTIME_ARTIFACTS seems to
      # copy symlinks verbatim, so resolve them now.
      set(lib_var_real "${lib_var}_REAL")
      get_filename_component(${lib_var_real} "${${lib_var}}" REALPATH)
      # find_library makes cache variables, so we have to override it.
      set(${lib_var} "${${lib_var_real}}" CACHE FILEPATH "" FORCE)

      if(OPENDDS_CMAKE_VERBOSE)
        message(STATUS "${lib_var}: ${${lib_var_real}}")
      endif()

      if(MSVC AND NOT OPENDDS_STATIC)
        # find_library finds the ".lib" file on Windows, but if OpenDDS is not
        # static we also need the ".dll" file for IMPORTED_LOCATION and
        # IMPORTED_RUNTIME_ARTIFACTS to work correctly.
        find_file("${lib_var}_DLL" "${lib_file_base}.dll" HINTS "${lib_dir}")
      endif()
    elseif(NOT DEFINED ${found_var})
      if(OPENDDS_CMAKE_VERBOSE)
        message(STATUS "${lib_var}: NOT FOUND")
      endif()
      set(${found_var} FALSE PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

# Needs to be a macro because of select_library_configurations
macro(_opendds_find_our_libraries lib_group libs)
  if(MSVC)
    _opendds_find_our_libraries_for_config(${lib_group} "${libs}" "RELEASE" "")
    _opendds_find_our_libraries_for_config(${lib_group} "${libs}" "DEBUG" "d")
  elseif(OPENDDS_DEBUG)
    _opendds_find_our_libraries_for_config(${lib_group} "${libs}" "DEBUG" "")
  else()
    _opendds_find_our_libraries_for_config(${lib_group} "${libs}" "RELEASE" "")
  endif()

  foreach(_lib ${libs})
    string(TOUPPER ${_lib} _lib_var)
    if(NOT DEFINED "${_lib_var}_DEPS")
      message(FATAL_ERROR "Library ${_lib} is missing a dependency list called ${_lib_var}_DEPS!")
    endif()
    select_library_configurations(${_lib_var})
  endforeach()
endmacro()

function(_opendds_found_required_deps found_var required_deps)
  set(missing_deps)
  foreach(dep ${required_deps})
    if(NOT ${dep})
      list(APPEND missing_deps ${dep})
    endif()
  endforeach()

  if(missing_deps)
    message(SEND_ERROR "Missing required dependencies ${missing_deps}")
    set("${found_var}" FALSE PARENT_SCOPE)
  else()
    set("${found_var}" TRUE PARENT_SCOPE)
  endif()
endfunction()

function(_opendds_add_target_binary target path)
  if(NOT TARGET ${target} AND EXISTS "${path}")
    add_executable(${target} IMPORTED)
    set_target_properties(${target}
      PROPERTIES
        IMPORTED_LOCATION "${path}"
    )
  endif()
endfunction()

function(_opendds_add_library_group lib_group_name libs has_mononym)
  string(TOUPPER ${lib_group_name} lib_group_var_prefix)

  macro(add_target_library_config target var_prefix config)
    set(lib_var "${var_prefix}_LIBRARY_${config}")
    set(lib_file "${${lib_var}}")
    if(EXISTS "${lib_file}")
      set_property(TARGET ${target}
        APPEND PROPERTY
        IMPORTED_CONFIGURATIONS ${config}
      )

      # Set any extra compiler and linker options that are needed to use the
      # libraries.
      foreach(from_libs ALL "JUST_${lib_group_var_prefix}")
        foreach(kind COMPILE LINK)
          set(options_var "OPENDDS_${from_libs}_LIBS_INTERFACE_${kind}_OPTIONS")
          if(DEFINED ${options_var})
            set_property(TARGET ${target}
              APPEND PROPERTY "INTERFACE_${kind}_OPTIONS" "${${options_var}}")
          endif()
        endforeach()
      endforeach()

      set(imploc "${lib_file}")
      if(MSVC)
        set_target_properties(${target}
          PROPERTIES
            "IMPORTED_IMPLIB_${config}" "${lib_file}"
        )
        set(dll "${lib_var}_DLL")
        if(DEFINED "${dll}")
          set(imploc "${${dll}}")
        endif()
      endif()
      set_target_properties(${target}
        PROPERTIES
          "IMPORTED_LINK_INTERFACE_LANGUAGES_${config}" "CXX"
          "IMPORTED_LOCATION_${config}" "${imploc}"
      )
    endif()
  endmacro()

  macro(add_target_library target var_prefix include_dirs)
    if(NOT TARGET ${target} AND ${var_prefix}_LIBRARY_FOUND)
      add_library(${target} ${OPENDDS_LIBRARY_TYPE} IMPORTED)
      set_target_properties(${target}
        PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${include_dirs}"
          INTERFACE_LINK_LIBRARIES "${${var_prefix}_DEPS}"
          INTERFACE_COMPILE_DEFINITIONS "${${var_prefix}_COMPILE_DEFINITIONS}"
          INTERFACE_COMPILE_OPTIONS "${${var_prefix}_COMPILE_OPTIONS}"
      )

      add_target_library_config(${target} ${var_prefix} "RELEASE")
      add_target_library_config(${target} ${var_prefix} "DEBUG")
    endif()
  endmacro()

  foreach(lib ${libs})
    string(TOUPPER ${lib} var_prefix)

    if(has_mononym AND lib STREQUAL "${lib_group_name}")
      set(target "${lib_group_name}::${lib_group_name}")
    else()
      string(REPLACE "${lib_group_name}_" "${lib_group_name}::" target "${lib}")
    endif()

    if(DEFINED "${var_prefix}_INCLUDE_DIRS")
      set(include_dirs "${${var_prefix}_INCLUDE_DIRS}")
    else()
      set(include_dirs "${${lib_group_var_prefix}_INCLUDE_DIRS}")
    endif()

    add_target_library(${target} ${var_prefix} "${include_dirs}")
  endforeach()
endfunction()

function(_opendds_get_library_var_prefix scoped_name var_prefix_var)
  if(scoped_name STREQUAL "ACE::ACE")
    set(var_prefix "ACE")
  elseif(scoped_name STREQUAL "TAO::TAO")
    set(var_prefix "TAO")
  else()
    string(TOUPPER ${scoped_name} var_prefix)
    string(REPLACE "::" "_" var_prefix "${var_prefix}")
  endif()

  set(${var_prefix_var} ${var_prefix} PARENT_SCOPE)
endfunction()

function(opendds_get_library_dependencies deps_var lib)
  set(libs "${lib}")
  list(APPEND libs ${ARGN})
  set(deps "${${deps_var}}")
  foreach(lib ${libs})
    if(NOT ${lib} IN_LIST deps)
      string(REGEX MATCH "^(OpenDDS|ACE|TAO)::" re_out "${lib}")
      if(CMAKE_MATCH_1)
        set(ace_tao_opendds ${CMAKE_MATCH_1})
        _opendds_get_library_var_prefix(${lib} var_prefix)
        set(dep_list_name "${var_prefix}_DEPS")
        if(DEFINED ${dep_list_name})
          opendds_get_library_dependencies(deps ${${dep_list_name}})
        endif()
        list(APPEND deps ${lib})
      endif()
    endif()
  endforeach()

  list(REMOVE_DUPLICATES deps)
  set(${deps_var} "${deps}" PARENT_SCOPE)
endfunction()
