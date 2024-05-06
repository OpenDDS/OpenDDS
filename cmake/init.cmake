# Initializes various variables for paths, features, and options for
# opendds_target_sources.
#
# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

if(_OPENDDS_INIT_CMAKE)
  return()
endif()
set(_OPENDDS_INIT_CMAKE TRUE)

# This is required for CMake <=3.4 for cmake_parse_arguments. Remove this when
# we no longer need to support those versions.
include(CMakeParseArguments)

include("${CMAKE_CURRENT_LIST_DIR}/opendds_version.cmake")

function(_opendds_detect_ace)
  if(OPENDDS_CMAKE_VERBOSE)
    set(path "${OPENDDS_ACE}/bin/MakeProjectCreator/config/default.features")
    if(NOT EXISTS "${path}")
      message(STATUS "${path} does not exist, assuming defaults")
      return()
    endif()
    message(STATUS "Getting features from ACE at ${OPENDDS_ACE}")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
  endif()

  file(STRINGS "${path}" config_text)
  foreach(name_value ${config_text})
    if(name_value MATCHES "([^=]+)=([^\n]+)")
      set(name "${CMAKE_MATCH_1}")
      set(value "${CMAKE_MATCH_2}")
      if(OPENDDS_CMAKE_VERBOSE)
        message(STATUS "${name}=${value}")
      endif()
      set("_opendds_default_features_${name}" "${value}" PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

include("${CMAKE_CURRENT_LIST_DIR}/config.cmake" OPTIONAL RESULT_VARIABLE OPENDDS_CONFIG_CMAKE)
if(OPENDDS_USE_PREFIX_PATH)
  set(_OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
else()
  set(_OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
  if(DEFINED OPENDDS_ACE)
    if(NOT EXISTS "${OPENDDS_ACE}")
      message(SEND_ERROR "OPENDDS_ACE (${OPENDDS_ACE}) does not exist")
      return()
    endif()
  else()
    message(SEND_ERROR "OPENDDS_ACE must be defined")
    return()
  endif()
  get_filename_component(OPENDDS_ACE "${OPENDDS_ACE}" ABSOLUTE)

  if(DEFINED OPENDDS_TAO)
    if(NOT EXISTS "${OPENDDS_TAO}")
      message(SEND_ERROR "OPENDDS_TAO (${OPENDDS_TAO}) does not exist")
      return()
    endif()
  elseif(EXISTS "${OPENDDS_ACE}/TAO")
    set(OPENDDS_TAO "${OPENDDS_ACE}/TAO")
  elseif(EXISTS "${OPENDDS_ACE}/../TAO")
    set(OPENDDS_TAO "${OPENDDS_ACE}/../TAO")
  else()
    message(SEND_ERROR
      "OPENDDS_TAO not relative to OPENDDS_ACE (${OPENDDS_ACE}), so OPENDDS_TAO must be defined")
    return()
  endif()
  get_filename_component(OPENDDS_TAO "${OPENDDS_TAO}" ABSOLUTE)
endif()
if(NOT OPENDDS_CONFIG_CMAKE AND NOT ACE_IS_BEING_BUILT)
  _opendds_detect_ace()
endif()
get_filename_component(_OPENDDS_ROOT "${_OPENDDS_ROOT}" ABSOLUTE)

if(NOT DEFINED OPENDDS_INSTALL_LIB)
  set(OPENDDS_INSTALL_LIB "lib")
endif()

file(TO_CMAKE_PATH "${OPENDDS_ACE}" OPENDDS_ACE)
if(OPENDDS_MPC)
  file(TO_CMAKE_PATH "${OPENDDS_MPC}" OPENDDS_MPC)
endif()
file(TO_CMAKE_PATH "${OPENDDS_TAO}" OPENDDS_TAO)

if(NOT DEFINED DDS_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(DDS_ROOT "${_OPENDDS_ROOT}/share/dds")
    set(OPENDDS_INCLUDE_DIRS "${_OPENDDS_ROOT}/include")

  else()
    set(DDS_ROOT "${_OPENDDS_ROOT}")
    set(OPENDDS_INCLUDE_DIRS "${_OPENDDS_ROOT}")
  endif()

  if(NOT OPENDDS_IS_BEING_BUILT)
    set(OPENDDS_BIN_DIR "${_OPENDDS_ROOT}/bin")
    set(OPENDDS_LIB_DIR "${_OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")
  endif()
endif()

if(NOT DEFINED ACE_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(ACE_ROOT "${_OPENDDS_ROOT}/share/ace")
    set(ACE_INCLUDE_DIRS "${_OPENDDS_ROOT}/include")
    set(ACE_LIB_DIR "${_OPENDDS_ROOT}/${OPENDDS_INSTALL_LIB}")

  elseif(OPENDDS_ACE)
    set(ACE_ROOT "${OPENDDS_ACE}")
    set(ACE_INCLUDE_DIRS "${ACE_ROOT}")
    set(ACE_LIB_DIR "${ACE_ROOT}/lib")
  endif()

  set(ACE_BIN_DIR "${ACE_ROOT}/bin")
endif()
_opendds_get_version(OPENDDS_ACE_VERSION ACE "${ACE_ROOT}")

if(NOT DEFINED TAO_ROOT)
  if(OPENDDS_USE_PREFIX_PATH)
    set(TAO_ROOT "${_OPENDDS_ROOT}/share/tao")
    set(TAO_INCLUDE_DIR "${_OPENDDS_ROOT}/include")

  elseif(OPENDDS_TAO)
    set(TAO_ROOT "${OPENDDS_TAO}")
    set(TAO_INCLUDE_DIR "${OPENDDS_TAO}")
  endif()

  set(TAO_BIN_DIR "${ACE_BIN_DIR}")
  set(TAO_LIB_DIR "${ACE_LIB_DIR}")
  set(TAO_INCLUDE_DIRS
    "${TAO_INCLUDE_DIR}"
    "${TAO_INCLUDE_DIR}/orbsvcs"
  )
endif()
_opendds_get_version(OPENDDS_TAO_VERSION TAO "${TAO_ROOT}")

message(STATUS "Using OpenDDS ${OPENDDS_VERSION} at ${DDS_ROOT}")
if(DEFINED OPENDDS_MPC)
  message(STATUS "Using MPC at ${OPENDDS_MPC}")
else()
  message(STATUS "Not using MPC (not needed)")
endif()
message(STATUS "Using ACE ${OPENDDS_ACE_VERSION} at ${ACE_ROOT}")
message(STATUS "Using TAO ${OPENDDS_TAO_VERSION} at ${TAO_ROOT}")

function(_opendds_cxx_std_to_year out_var cxx_std)
  if(cxx_std STREQUAL 98)
    set(year 1998)
  else()
    math(EXPR year "2000 + ${cxx_std}")
  endif()
  set(${out_var} ${year} PARENT_SCOPE)
endfunction()

function(_opendds_cplusplus_to_year out_var cplusplus)
  math(EXPR year "${cplusplus} / 100")
  set(${out_var} ${year} PARENT_SCOPE)
endfunction()

function(_opendds_cxx_std_from_year out_var year)
  if(year STREQUAL 1998)
    set(std 98)
  else()
    math(EXPR std "${year} - 2000")
  endif()
  set(${out_var} ${std} PARENT_SCOPE)
endfunction()

function(_opendds_set_cxx_std)
  set(cplusplus_values 201103 201402 201703 202002 202302)
  set(test_cxx_std "${CMAKE_CURRENT_LIST_DIR}/test_cxx_std.cpp")
  set(temp_dir "${CMAKE_CURRENT_BINARY_DIR}/opendds_test_cxx_std")
  file(MAKE_DIRECTORY "${temp_dir}")

  # Get the latest known default compiler C++ standard
  set(default_cxx_std_year 1998)
  foreach(cplusplus IN LISTS cplusplus_values)
    try_compile(compiled
      "${temp_dir}/cplusplus_${cplusplus}"
      SOURCES "${test_cxx_std}"
      COMPILE_DEFINITIONS "-DOPENDDS_TEST_CPLUSPLUS=${cplusplus}L"
    )
    if(compiled)
      _opendds_cplusplus_to_year(default_cxx_std_year ${cplusplus})
    else()
      break()
    endif()
  endforeach()

  # Get the max C++ standard supported by the compiler
  set(max_cxx_std_year ${default_cxx_std_year})
  if(NOT CMAKE_VERSION VERSION_LESS "3.8.0")
    foreach(feature IN LISTS CMAKE_CXX_COMPILE_FEATURES)
      if(feature MATCHES "^cxx_std_(.*)$")
        _opendds_cxx_std_to_year(supported_year "${CMAKE_MATCH_1}")
        if(supported_year GREATER max_cxx_std_year)
          set(max_cxx_std_year "${supported_year}")
        endif()
      endif()
    endforeach()
  endif()

  # Get the min C++ standard that might be used
  set(explicit TRUE)
  if(DEFINED OPENDDS_CXX_STD)
    # User is overriding the standard for OpenDDS specifically
    _opendds_cxx_std_to_year(cxx_std_year ${OPENDDS_CXX_STD})
  elseif(DEFINED CMAKE_CXX_STANDARD)
    # User is overriding the standard globally
    _opendds_cxx_std_to_year(cxx_std_year ${CMAKE_CXX_STANDARD})
  else()
    set(explicit FALSE)
    set(cxx_std_year ${default_cxx_std_year})
  endif()

  # Get the C++ standard ACE requires
  set(ace_info "ACE ${OPENDDS_ACE_VERSION}")
  set(compiler_info "compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
  set(existing_cxx11 FALSE)
  if(OPENDDS_CXX11 OR (DEFINED _opendds_default_features_no_cxx11 AND
      _opendds_default_features_no_cxx11 STREQUAL "0"))
    set(existing_cxx11 TRUE)
  endif()
  set(can_build_cxx11 FALSE)
  if(ACE_IS_BEING_BUILT AND NOT cxx_std_year LESS 2011)
    set(can_build_cxx11 TRUE)
  endif()
  if(OPENDDS_ACE_VERSION VERSION_LESS "7.0.0")
    # What ACE 6 requires depends on the no_cxx11 feature
    set(ace_info "${ace_info} (cxx11=${existing_cxx11})")
    if(existing_cxx11 OR can_build_cxx11)
      set(ace_min_cxx_std_year 2011)
    else()
      set(ace_min_cxx_std_year 1998)
    endif()
  else()
    # What ACE 7+ requires depends on a check in ace/Global_Macros.h. We must
    # compile against that header using each standard we know of until it
    # compiles.
    set(includes "${ACE_INCLUDE_DIRS}")
    if(NOT EXISTS "${ACE_INCLUDE_DIRS}/ace/config.h")
      if(NOT ACE_IS_BEING_BUILT)
        message(FATAL_ERROR "ACE doesn't have a config.h.")
      endif()
      # Won't compile without a config.h, so make a fake one.
      set(include_dir "${temp_dir}/include")
      list(APPEND includes "${include_dir}")
      set(ace_dir "${include_dir}/ace")
      file(MAKE_DIRECTORY "${ace_dir}")
      file(WRITE "${ace_dir}/config.h" "#include <ace/${_OPENDDS_ACE_CONFIG_FILE}>\n")
    endif()
    foreach(try_cplusplus IN LISTS cplusplus_values)
      _opendds_cplusplus_to_year(try_year ${try_cplusplus})
      _opendds_cxx_std_from_year(try_std ${try_year})
      try_compile(compiled
        "${temp_dir}/ace_cxx_std_${try_cplusplus}"
        SOURCES "${test_cxx_std}"
        COMPILE_DEFINITIONS "-DOPENDDS_TEST_ACE_CXX_STD"
        CMAKE_FLAGS
          "-DCMAKE_CXX_STANDARD=${try_std}"
          "-DINCLUDE_DIRECTORIES=${includes}"
        OUTPUT_VARIABLE build_output
      )
      if(compiled)
        set(ace_min_cxx_std_year ${try_year})
        break()
      endif()
    endforeach()
    if(NOT DEFINED ace_min_cxx_std_year)
      message(FATAL_ERROR
        " Can't figure out required C++ standard for ${ace_info}\n"
        " Tried up to C++ ${try_year} on ${compiler_info}, last output:\n"
        " \n"
        " ${build_output}")
    endif()
  endif()

  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "default_cxx_std_year: ${default_cxx_std_year}")
    message(STATUS "max_cxx_std_year: ${max_cxx_std_year}")
    message(STATUS "cxx_std_year: ${cxx_std_year}")
    message(STATUS "existing_cxx11: ${existing_cxx11}")
    message(STATUS "can_build_cxx11: ${can_build_cxx11}")
    message(STATUS "ace_min_cxx_std_year: ${ace_min_cxx_std_year}")
  endif()

  # See if ACE requires a later standard
  if(cxx_std_year LESS ace_min_cxx_std_year)
    set(msg "${ace_info} requires at least C++ ${ace_min_cxx_std_year}")
    if(NOT explicit AND max_cxx_std_year LESS ace_min_cxx_std_year)
      message(FATAL_ERROR "${msg}, but ${compiler_info} only supports up to "
        "C++ ${max_cxx_std_year}.")
    endif()
    message(STATUS "${compiler_info} would use ${cxx_std_year}, but ${msg}. Raising "
      "requirment to that.")
    set(cxx_std_year ${ace_min_cxx_std_year})
  endif()

  _opendds_cxx_std_from_year(cxx_std ${cxx_std_year})
  set(OPENDDS_CXX_STD ${cxx_std} CACHE STRING
    "Minimum required C++ standard (same values as CMAKE_CXX_STANDARD)" FORCE)
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "OPENDDS_CXX_STD: ${OPENDDS_CXX_STD}")
  endif()
  set(OPENDDS_CXX_STD_YEAR ${cxx_std_year} CACHE STRING
    "Minimum required C++ standard year (do not set mannually)" FORCE)
endfunction()

if(NOT DEFINED OPENDDS_CXX_STD_YEAR)
  _opendds_set_cxx_std()
endif()

function(_opendds_cxx_std target scope)
  if(OPENDDS_CXX_STD AND NOT (scope STREQUAL INTERFACE AND CMAKE_VERSION VERSION_LESS "3.11.0"))
    target_compile_features(${target} ${scope} "cxx_std_${OPENDDS_CXX_STD}")
  endif()
endfunction()

set(_OPENDDS_ALL_FEATURES)
set(_OPENDDS_FEATURE_VARS)
set(_OPENDDS_MPC_FEATURES)
function(_opendds_feature name default_value)
  set(no_value_options MPC CONFIG)
  set(single_value_options MPC_NAME MPC_INVERTED_NAME DOC TYPE)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})
  if(NOT DEFINED arg_TYPE)
    set(arg_TYPE BOOL)
  endif()

  # Get and record various names
  string(TOLOWER "${name}" lowercase_name)
  list(APPEND _OPENDDS_ALL_FEATURES "${lowercase_name}")
  set(config_name "OPENDDS_CONFIG_${name}")
  set(name "OPENDDS_${name}")
  list(APPEND _OPENDDS_FEATURE_VARS "${name}")

  # Get MPC name and values
  if(arg_MPC OR arg_MPC_NAME OR arg_MPC_INVERTED_NAME)
    if(NOT DEFINED arg_MPC_NAME)
      if(DEFINED arg_MPC_INVERTED_NAME)
        set(arg_MPC_NAME "${arg_MPC_INVERTED_NAME}")
      else()
        set(arg_MPC_NAME "${lowercase_name}")
      endif()
    endif()
    if(DEFINED arg_MPC_INVERTED_NAME)
      set(mpc_true 0)
      set(mpc_false 1)
    else()
      set(mpc_true 1)
      set(mpc_false 0)
    endif()

    # See if it was set in ACE's default.features
    set(mpc_var "_opendds_default_features_${arg_MPC_NAME}")
    if(DEFINED ${mpc_var})
      set(mpc_val "${${mpc_var}}")
      if(mpc_val STREQUAL mpc_true)
        set(default_value TRUE)
      else()
        set(default_value FALSE)
      endif()
    endif()
  endif()

  # Make sure the value is set
  set("${name}" "${default_value}" CACHE "${arg_TYPE}" "${arg_DOC}")
  set(value "${${name}}")

  # Set MPC feature name and value in case we're creating default.features
  if(arg_MPC_NAME)
    if(value)
      set(mpc_feature "${arg_MPC_NAME}=${mpc_true}")
    else()
      set(mpc_feature "${arg_MPC_NAME}=${mpc_false}")
    endif()
    list(APPEND _OPENDDS_MPC_FEATURES "${mpc_feature}")
  endif()

  # Set values for OpenDDSConfig.h
  if(arg_CONFIG)
    if(arg_TYPE STREQUAL BOOL)
      if(value)
        set(config_value 1)
      else()
        set(config_value 0)
      endif()
    else()
      set(config_value "${value}")
    endif()
    set("${config_name}" "${config_value}" CACHE INTERNAL "" FORCE)
  endif()

  set(_OPENDDS_ALL_FEATURES "${_OPENDDS_ALL_FEATURES}" PARENT_SCOPE)
  set(_OPENDDS_FEATURE_VARS "${_OPENDDS_FEATURE_VARS}" PARENT_SCOPE)
  set(_OPENDDS_MPC_FEATURES "${_OPENDDS_MPC_FEATURES}" PARENT_SCOPE)
endfunction()

# OpenDDS Features
_opendds_feature(BUILT_IN_TOPICS ON CONFIG DOC "Enables built-in-topics (BITs)")
_opendds_feature(OBJECT_MODEL_PROFILE ON CONFIG DOC "Allows using presentation group QoS")
_opendds_feature(PERSISTENCE_PROFILE ON CONFIG
  DOC "Allows using the durability and durability service QoS")
_opendds_feature(OWNERSHIP_PROFILE ON CONFIG
  DOC "Allows history depth QoS and implies OPENDDS_OWNERSHIP_KIND_EXCLUSIVE")
_opendds_feature(OWNERSHIP_KIND_EXCLUSIVE "${OPENDDS_OWNERSHIP_PROFILE}" CONFIG
  DOC "Allows the EXCLUSIVE ownership QoS")
_opendds_feature(CONTENT_SUBSCRIPTION ON CONFIG
  DOC "Implies OPENDDS_CONTENT_FILTERED_TOPIC, OPENDDS_MULTI_TOPIC, and OPENDDS_QUERY_CONDITION")
_opendds_feature(CONTENT_FILTERED_TOPIC "${OPENDDS_CONTENT_SUBSCRIPTION}" CONFIG
  DOC "Allows using ContentFilteredTopic")
_opendds_feature(MULTI_TOPIC "${OPENDDS_CONTENT_SUBSCRIPTION}" CONFIG DOC "Allows using MultiTopic")
_opendds_feature(QUERY_CONDITION "${OPENDDS_CONTENT_SUBSCRIPTION}" CONFIG
  DOC "Allows using QueryCondition")
_opendds_feature(SUPPRESS_ANYS ON CONFIG MPC_NAME dds_suppress_anys
  DOC "Default for opendds_target_sources(SUPPRESS_ANYS)")
_opendds_feature(SECURITY OFF CONFIG DOC "Build with RTPS Security support")
_opendds_feature(SAFETY_PROFILE OFF CONFIG MPC_INVERTED_NAME no_opendds_safety_profile
  DOC "Build using Safety Profile (Not for CMake-built OpenDDS)")
_opendds_feature(COVERAGE OFF MPC_INVERTED_NAME dds_non_coverage)
_opendds_feature(BOOTTIME_TIMERS OFF CONFIG DOC "Use CLOCK_BOOTTIME for timers")

# ACE Features
_opendds_feature(VERSIONED_NAMESPACE OFF MPC DOC "Namespaces include versions")
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  _opendds_feature(DEBUG ON MPC DOC "(Not for CMake-built OpenDDS)")
  _opendds_feature(OPTIMIZE OFF MPC DOC "(Not for CMake-built OpenDDS)")
else()
  _opendds_feature(DEBUG OFF MPC DOC "(Not for CMake-built OpenDDS)")
  _opendds_feature(OPTIMIZE ON MPC DOC "(Not for CMake-built OpenDDS)")
endif()
_opendds_feature(INLINE ON MPC DOC "(Not for CMake-built OpenDDS)")
if(BUILD_SHARED_LIBS)
  _opendds_feature(STATIC OFF MPC DOC "(Not for CMake-built OpenDDS)")
else()
  _opendds_feature(STATIC ON MPC DOC "(Not for CMake-built OpenDDS)")
endif()
_opendds_feature(XERCES3 "${OPENDDS_SECURITY}" MPC TYPE PATH
  DOC "Build with Xerces XML parser, needed for security and QoS XML Handler")
_opendds_feature(IPV6 OFF MPC DOC "Build with IPv6 support")
if(OPENDDS_CXX_STD_YEAR LESS 2011)
  set(_opendds_cxx11_default OFF)
else()
  set(_opendds_cxx11_default ON)
endif()
_opendds_feature(CXX11 "${_opendds_cxx11_default}" MPC_INVERTED_NAME no_cxx11
  DOC "Build assumes C++11 support")

# TAO Features
_opendds_feature(TAO_IIOP ON MPC_INVERTED_NAME tao_no_iiop)
_opendds_feature(TAO_OPTIMIZE_COLLOCATED_INVOCATIONS ON MPC)

option(OPENDDS_CMAKE_VERBOSE "Print verbose output when loading the OpenDDS Config Package" OFF)
if("all" IN_LIST OPENDDS_CMAKE_VERBOSE)
  set(OPENDDS_CMAKE_VERBOSE
    components
    imports
    opendds_target_sources
    CACHE STRING "" FORCE)
endif()
option(OPENDDS_DEFAULT_NESTED "Require topic types to be declared explicitly" ON)
option(OPENDDS_FILENAME_ONLY_INCLUDES "No directory info in generated #includes." OFF)
set(OPENDDS_DEFAULT_SCOPE "PRIVATE" CACHE STRING "Default scope for opendds_target_sources")
set_property(CACHE OPENDDS_DEFAULT_SCOPE PROPERTY STRINGS "PUBLIC" "PRIVATE" "INTERFACE")
option(OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER
  "Always generate an export header for libraries" OFF)
# This is off because it's not compatible with a possible existing usage of
# target_link_libraries that doesn't specify a scope:
# "All uses of target_link_libraries with a target must be either all-keyword
# or all-plain."
# TODO: Make this default ON in v4.0
option(OPENDDS_AUTO_LINK_DCPS
  "Automatically link dependencies to the target of opendds_target_sources" OFF)
# This is off by default because it could cause "Cannot find source file"
# errors on `TypeSupport.idl` files generated in a another directory.
# TODO: Make this default ON in v4.0
option(OPENDDS_USE_CORRECT_INCLUDE_SCOPE
  "Include using SCOPE specified in opendds_target_sources" OFF)

macro(_opendds_save_cache name type value)
  list(APPEND _opendds_save_cache_vars ${name})
  set(_opendds_save_cache_${name}_type ${type})
  set(_opendds_save_cache_${name}_value "${${name}}")
  set(${name} "${value}" CACHE ${type} "" FORCE)
endmacro()

macro(_opendds_restore_cache)
  foreach(name ${_opendds_save_cache_vars})
    set(${name} "${_opendds_save_cache_${name}_value}" CACHE
      "${_opendds_save_cache_${name}_type}" "" FORCE)
    unset(_opendds_save_cache_${name}_type)
    unset(_opendds_save_cache_${name}_value)
  endforeach()
  unset(_opendds_save_cache_vars)
endmacro()

function(_opendds_pop_list list_var)
  set(list "${${list_var}}")
  list(LENGTH list len)
  if(len GREATER 0)
    math(EXPR last "${len} - 1")
    list(REMOVE_AT list "${last}")
    set("${list_var}" "${list}" PARENT_SCOPE)
  endif()
endfunction()

function(_opendds_path_list path_list_var)
  if("APPEND" IN_LIST ARGN)
    set(path_list "${${path_list_var}}")
    list(REMOVE_ITEM ARGN APPEND)
  else()
    set(path_list)
  endif()

  if(WIN32)
    set(delimiter ";")
  else()
    set(delimiter ":")
  endif()

  foreach(path ${ARGN})
    if(path_list AND NOT path_list MATCHES "${delimiter}$")
      set(path_list "${path_list}${delimiter}")
    endif()
    set(path_list "${path_list}${path}")
  endforeach()

  set("${path_list_var}" "${path_list}" PARENT_SCOPE)
endfunction()

if(OPENDDS_STATIC)
  set(OPENDDS_LIBRARY_TYPE STATIC)
else()
  set(OPENDDS_LIBRARY_TYPE SHARED)
endif()

if(OPENDDS_COVERAGE)
  list(APPEND OPENDDS_JUST_OPENDDS_LIBS_INTERFACE_COMPILE_OPTIONS "--coverage")
  list(APPEND OPENDDS_JUST_OPENDDS_LIBS_INTERFACE_LINK_OPTIONS "--coverage")
endif()

if(DEFINED OPENDDS_SANITIZER_COMPILER_ARGS)
  list(APPEND OPENDDS_ALL_LIBS_INTERFACE_COMPILE_OPTIONS "${OPENDDS_SANITIZER_COMPILER_ARGS}")
endif()
if(DEFINED OPENDDS_SANITIZER_LINKER_ARGS)
  list(APPEND OPENDDS_ALL_LIBS_INTERFACE_LINK_OPTIONS "${OPENDDS_SANITIZER_LINKER_ARGS}")
endif()

if(DEFINED OPENDDS_ACE_TAO_HOST_TOOLS)
  set(_OPENDDS_ACE_HOST_TOOLS "${OPENDDS_ACE_TAO_HOST_TOOLS}/bin" CACHE INTERNAL "")
  set(_OPENDDS_TAO_HOST_TOOLS "${OPENDDS_ACE_TAO_HOST_TOOLS}/bin" CACHE INTERNAL "")
endif()
if(DEFINED OPENDDS_HOST_TOOLS)
  set(_OPENDDS_OPENDDS_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/bin" CACHE INTERNAL "")
  if(NOT DEFINED OPENDDS_ACE_TAO_HOST_TOOLS)
    if(IS_DIRECTORY "${OPENDDS_HOST_TOOLS}/ace_tao/bin")
      set(_OPENDDS_ACE_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/ace_tao/bin" CACHE INTERNAL "")
      set(_OPENDDS_TAO_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/ace_tao/bin" CACHE INTERNAL "")
    else()
      set(_OPENDDS_ACE_HOST_TOOLS "${_OPENDDS_OPENDDS_HOST_TOOLS}" CACHE INTERNAL "")
      set(_OPENDDS_TAO_HOST_TOOLS "${_OPENDDS_OPENDDS_HOST_TOOLS}" CACHE INTERNAL "")
    endif()
  endif()
endif()

if(NOT DEFINED OPENDDS_SUPPORTS_SHMEM)
  if(APPLE)
    set(OPENDDS_SUPPORTS_SHMEM FALSE)
  else()
    set(OPENDDS_SUPPORTS_SHMEM TRUE)
  endif()
endif()

# This should be in ace_group.cmake, but it's needed by build_ace_tao.cmake.
if(OPENDDS_XERCES3)
  find_package(XercesC PATHS "${OPENDDS_XERCES3}" NO_DEFAULT_PATH QUIET)
  if(NOT XercesC_FOUND)
    find_package(XercesC QUIET)
  endif()
  if(NOT XercesC_FOUND)
    message(FATAL_ERROR "Could not find XercesC")
  endif()
  get_filename_component(_opendds_xerces3_for_ace "${XercesC_INCLUDE_DIRS}" DIRECTORY)
  set(_OPENDDS_XERCES3_FOR_ACE "${_opendds_xerces3_for_ace}" CACHE PATH "" FORCE)
endif()
