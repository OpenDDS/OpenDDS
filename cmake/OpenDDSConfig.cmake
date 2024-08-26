# This is what CMake evaluates when find_package(OpenDDS) is called. It should
# find ACE/TAO and OpenDDS according to COMPONENTS passed and other information
# like OpenDDS_ROOT, make those libraries and executables available, and make
# opendds_targets_sources available if possible.
#
# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

cmake_minimum_required(VERSION 3.3...3.27)

if(OPENDDS_CMAKE_VERBOSE)
  message(STATUS "find_package(OpenDDS) called from ${PROJECT_NAME}")
endif()
set(_opendds_old_cmake_message_indent "${CMAKE_MESSAGE_INDENT}")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

include("${CMAKE_CURRENT_LIST_DIR}/init.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ace_group.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/tao_group.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/opendds_group.cmake")

# The COMPONENTS options of find_pacakge optionally define what libraries,
# executables, and features that are required.
function(_opendds_process_components)
  set(debug FALSE)
  if(components IN_LIST OPENDDS_CMAKE_VERBOSE)
    set(debug TRUE)
  endif()

  # Process components that were passed
  set(request_failed FALSE)
  set(load_defaults TRUE)
  set(find_targets)
  set(required_targets)
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "Processing find_package(COMPONENTS)")
  endif()
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  function(normalize_bool dest value)
    if(value)
      set("${dest}" TRUE PARENT_SCOPE)
    else()
      set("${dest}" FALSE PARENT_SCOPE)
    endif()
  endfunction()
  macro(check_feature name value)
    if(debug)
      message(STATUS "Feature was requested: ${name}=${value}")
    endif()
    if("${name}" IN_LIST _OPENDDS_ALL_FEATURES)
      if(required)
        string(TOUPPER "${name}" feature_name)
        set(feature_name "OPENDDS_${feature_name}")
        normalize_bool(requested_value "${value}")
        normalize_bool(actual_value "${${feature_name}}")
        if(NOT actual_value STREQUAL requested_value)
          message(SEND_ERROR
            "Requested feature ${name}=${requested_value}, but it's ${actual_value}")
          set(request_failed TRUE)
        endif()
      else()
        message(SEND_ERROR "Features like ${name}=${value} can't be optional")
        set(request_failed TRUE)
      endif()
    else()
      message(SEND_ERROR "Invalid feature requested named ${name}")
      set(request_failed TRUE)
    endif()
  endmacro()
  foreach(component ${OpenDDS_FIND_COMPONENTS})
    set(required "${OpenDDS_FIND_REQUIRED_${component}}")
    if(component STREQUAL "NO_DEFAULTS")
      set(load_defaults FALSE)
    elseif(component IN_LIST _OPENDDS_ALL_FEATURES)
      check_feature("${component}" TRUE)
    elseif(component MATCHES "^([^=]+)=(.*)$")
      if(CMAKE_VERSION VERSION_LESS "3.9.0")
        message(SEND_ERROR
          "CMake version must be at least 3.9 to use FEATURE_NAME=FEATURE_VALUE syntax")
        set(request_failed TRUE)
      else()
        set(name "${CMAKE_MATCH_1}")
        set(value "${CMAKE_MATCH_2}")
        check_feature("${name}" "${value}")
      endif()
    elseif(required AND NOT component IN_LIST _OPENDDS_ALL_TARGETS)
      message(SEND_ERROR "Unknown required component requested ${component}")
      set(request_failed TRUE)
    elseif(NOT TARGET "${${component}}")
      if(debug)
        message(STATUS "Missing target was requested: ${component}, required: ${required}")
      endif()
      list(APPEND find_targets "${component}")
      if(required)
        list(APPEND required_targets "${component}")
      endif()
    endif()
  endforeach()

  # Set defaults
  if(load_defaults)
    list(APPEND find_targets ${_OPENDDS_ALL_TARGETS})
    foreach(group ${_OPENDDS_ALL_GROUPS})
      string(TOUPPER "${group}" upper)
      list(APPEND required_targets ${_OPENDDS_${upper}_DEFAULT_REQUIRED})
    endforeach()
  endif()
  if(find_targets)
    list(REMOVE_DUPLICATES find_targets)
  endif()
  if(required_targets)
    list(REMOVE_DUPLICATES required_targets)
  endif()

  # Expand dependencies and convert to var names that can be passed to
  # _opendds_found_required_deps
  macro(process_target_list kind convert_to_var)
    opendds_get_library_dependencies("all_${kind}_targets" ${${kind}_targets} PASSTHROUGH)
    foreach(group ${_OPENDDS_ALL_GROUPS})
      string(TOLOWER "${group}" lower_group)
      set("${lower_group}_${kind}")
      set("${lower_group}_${kind}_libs")
      set("${lower_group}_${kind}_exes")
    endforeach()

    foreach(target ${all_${kind}_targets})
      set(found FALSE)
      foreach(group ${_OPENDDS_ALL_GROUPS})
        string(TOUPPER "${group}" upper_group)
        if(target IN_LIST "_OPENDDS_${upper_group}_ALL_TARGETS")
          set(found TRUE)
          string(TOLOWER "${group}" lower_group)
          set(add_to_list "${target}")
          if(${convert_to_var})
            if(${target} STREQUAL "${group}::${group}")
              set(add_to_list "${group}")
            else()
              string(REPLACE "::" "_" add_to_list "${target}")
            endif()
          endif()
          if(target IN_LIST _OPENDDS_ALL_LIBRARIES)
            if(${convert_to_var})
              set(add_to_list "${add_to_list}_LIBRARY")
            endif()
            string(REPLACE "${group}::" "" add_to_libs "${add_to_list}")
            list(APPEND "${lower_group}_${kind}_libs" "${add_to_libs}")
          elseif(target IN_LIST _OPENDDS_ALL_EXECUTABLES)
            if(${convert_to_var})
              set(add_to_list "_OPENDDS_${add_to_list}")
            endif()
            string(REPLACE "${group}::" "" add_to_exes "${add_to_list}")
            list(APPEND "${lower_group}_${kind}_exes" "${add_to_exes}")
          endif()
          if(${convert_to_var})
            string(TOUPPER "${add_to_list}" add_to_list)
          endif()
          list(APPEND "${lower_group}_${kind}" "${add_to_list}")
        endif()
        if(found)
          break()
        endif()
      endforeach()
    endforeach()
  endmacro()
  process_target_list("find" FALSE)
  process_target_list("required" TRUE)

  set(_opendds_required_targets "${required_targets}" PARENT_SCOPE)
  set(_opendds_request_failed "${request_failed}" PARENT_SCOPE)
  foreach(name
        ace_find_libs
        ace_find_exes
        ace_required
        tao_find_libs
        tao_find_exes
        tao_required
        opendds_find_libs
        opendds_find_exes
        opendds_required
      )
    if("${name}")
      list(REMOVE_DUPLICATES "${name}")
    endif()
    if(debug)
      message(STATUS "${name}: ${${name}}")
    endif()
    if(name MATCHES "^opendds")
      set("_${name}" "${${name}}" PARENT_SCOPE)
    else()
      set("_opendds_${name}" "${${name}}" PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

_opendds_process_components()
if(_opendds_request_failed)
  set(OpenDDS_FOUND FALSE)
  set(CMAKE_MESSAGE_INDENT "${_opendds_cmake_message_indent}")
  return()
endif()

if(OPENDDS_CMAKE_VERBOSE)
  message(STATUS "Finding and adding libraries and executables")
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

if(_opendds_ace_find_libs OR _opendds_ace_find_exes)
  _opendds_find_group_targets(ACE "${_opendds_ace_find_libs}" "${_opendds_ace_find_exes}")
  _opendds_found_required_deps(OpenDDS_FOUND "${_opendds_ace_required}")
  if(OpenDDS_FOUND)
    _opendds_import_group_targets(ACE "${_opendds_ace_find_libs}" "${_opendds_ace_find_exes}")
  else()
    set(CMAKE_MESSAGE_INDENT "${_opendds_cmake_message_indent}")
    return()
  endif()
endif()

if(_opendds_tao_find_libs OR _opendds_tao_find_exes)
  _opendds_find_group_targets(TAO "${_opendds_tao_find_libs}" "${_opendds_tao_find_exes}")
  _opendds_found_required_deps(OpenDDS_FOUND "${_opendds_tao_required}")
  if(OpenDDS_FOUND)
    _opendds_import_group_targets(TAO "${_opendds_tao_find_libs}" "${_opendds_tao_find_exes}")
  else()
    set(CMAKE_MESSAGE_INDENT "${_opendds_cmake_message_indent}")
    return()
  endif()
endif()

if(_OPENDDS_CMAKE_BUILT_AND_INSTALLED)
  include("${CMAKE_CURRENT_LIST_DIR}/opendds_targets.cmake")
  set(OpenDDS_FOUND TRUE)
  foreach(_tgt ${_opendds_required_targets})
    if(NOT TARGET ${_tgt})
      message(SEND_ERROR "${_tgt} was not in installed OpenDDS/ACE/TAO")
      set(OpenDDS_FOUND FALSE)
    endif()
  endforeach()
elseif(_opendds_find_libs OR _opendds_find_exes)
  _opendds_find_group_targets(OpenDDS "${_opendds_find_libs}" "${_opendds_find_exes}")
  _opendds_found_required_deps(OpenDDS_FOUND "${_opendds_required}")
  if(OpenDDS_FOUND)
    _opendds_import_group_targets(OpenDDS "${_opendds_find_libs}" "${_opendds_find_exes}")
  else()
    set(CMAKE_MESSAGE_INDENT "${_opendds_cmake_message_indent}")
    return()
  endif()
endif()

if(NOT TARGET OpenDDS::OpenDDS)
  set(_opendds_core_libs
    OpenDDS::Dcps
    OpenDDS::Multicast
    OpenDDS::Rtps
    OpenDDS::Rtps_Udp
    OpenDDS::InfoRepoDiscovery
    OpenDDS::Shmem
    OpenDDS::Tcp
  )
  if(OPENDDS_SECURITY)
    list(APPEND _opendds_core_libs OpenDDS::Security)
  endif()

  set(_found_all TRUE)
  foreach(_lib ${_opendds_core_libs})
    if(NOT TARGET "${_lib}")
      set(_found_all FALSE)
      break()
    endif()
  endforeach()

  if(_found_all)
    add_library(OpenDDS::OpenDDS INTERFACE IMPORTED)
    set_target_properties(OpenDDS::OpenDDS
      PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_opendds_core_libs}")
  endif()
endif()

if(NOT TARGET OpenDDS::TestUtils AND DEFINED OPENDDS_SOURCE_DIR)
  add_library(OpenDDS::TestUtils INTERFACE IMPORTED)
  target_include_directories(OpenDDS::TestUtils INTERFACE "${OPENDDS_SOURCE_DIR}")
endif()

if(TARGET TAO::tao_idl OR TARGET OpenDDS::opendds_idl)
  include("${CMAKE_CURRENT_LIST_DIR}/opendds_target_sources.cmake")
endif()

set(CMAKE_MESSAGE_INDENT "${_opendds_cmake_message_indent}")
