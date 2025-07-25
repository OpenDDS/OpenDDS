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
include("${CMAKE_CURRENT_LIST_DIR}/opendds_utils.cmake")

enable_language(C CXX)

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

set(_opendds_config_cmake "${CMAKE_CURRENT_LIST_DIR}/config.cmake")
include("${_opendds_config_cmake}" OPTIONAL RESULT_VARIABLE _opendds_found_config_cmake)
if(_opendds_found_config_cmake)
  message(STATUS "OpenDDS config.cmake (${_opendds_config_cmake}) found")
else()
  message(STATUS "OpenDDS config.cmake (${_opendds_config_cmake}) NOT found")
endif()

if(DEFINED OPENDDS_ACE)
  get_filename_component(OPENDDS_ACE "${OPENDDS_ACE}" ABSOLUTE)
  if(NOT EXISTS "${OPENDDS_ACE}")
    message(SEND_ERROR "OPENDDS_ACE (${OPENDDS_ACE}) does not exist")
    return()
  endif()
elseif(OPENDDS_USE_PREFIX_PATH)
  set(OPENDDS_ACE_USE_PREFIX_PATH TRUE)
else()
  message(SEND_ERROR "OPENDDS_ACE must be defined")
  if(NOT _opendds_found_config_cmake AND NOT OPENDDS_IS_BEING_BUILT)
    message(SEND_ERROR "Maybe this OpenDDS needs to be built?")
  endif()
  return()
endif()

if(DEFINED OPENDDS_TAO)
  get_filename_component(OPENDDS_TAO "${OPENDDS_TAO}" ABSOLUTE)
  if(NOT EXISTS "${OPENDDS_TAO}")
    message(SEND_ERROR "OPENDDS_TAO (${OPENDDS_TAO}) does not exist")
    return()
  endif()
elseif(EXISTS "${OPENDDS_ACE}/TAO")
  set(OPENDDS_TAO "${OPENDDS_ACE}/TAO")
elseif(EXISTS "${OPENDDS_ACE}/../TAO")
  set(OPENDDS_TAO "${OPENDDS_ACE}/../TAO")
elseif(OPENDDS_USE_PREFIX_PATH)
  set(OPENDDS_TAO_USE_PREFIX_PATH TRUE)
else()
  message(SEND_ERROR
    "OPENDDS_TAO not relative to OPENDDS_ACE (${OPENDDS_ACE}), so OPENDDS_TAO must be defined")
  return()
endif()
if(NOT _opendds_found_config_cmake AND NOT ACE_IS_BEING_BUILT)
  _opendds_detect_ace()
endif()

if(NOT DEFINED OPENDDS_BUILT_USING_CMAKE AND OPENDDS_IS_BEING_BUILT)
  set(OPENDDS_BUILT_USING_CMAKE TRUE)
endif()

if(NOT DEFINED OPENDDS_INSTALL_LIB)
  set(OPENDDS_INSTALL_LIB "lib")
endif()

file(TO_CMAKE_PATH "${OPENDDS_ACE}" OPENDDS_ACE)
if(OPENDDS_MPC)
  file(TO_CMAKE_PATH "${OPENDDS_MPC}" OPENDDS_MPC)
endif()
file(TO_CMAKE_PATH "${OPENDDS_TAO}" OPENDDS_TAO)

if(OPENDDS_USE_PREFIX_PATH)
  set(_OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
else()
  set(_OPENDDS_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
endif()
get_filename_component(_OPENDDS_ROOT "${_OPENDDS_ROOT}" ABSOLUTE)

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
  if(OPENDDS_ACE_USE_PREFIX_PATH)
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
  if(OPENDDS_TAO_USE_PREFIX_PATH)
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
  if(cxx_std STREQUAL 98 OR cxx_std STREQUAL 03) # 2003 is not used in CMake's CXX_STANDARD property
    set(year 1998)
  else()
    math(EXPR year "2000 + ${cxx_std}")
  endif()
  set(${out_var} ${year} PARENT_SCOPE)
endfunction()

function(_opendds_cplusplus_to_year out_var cplusplus)
  math(EXPR year "${cplusplus} / 100")
  if(year MATCHES 1997)
    set(year 1998)
  endif()
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

set(_opendds_try_compile_dir "${CMAKE_CURRENT_BINARY_DIR}/opendds_try_compile")
file(MAKE_DIRECTORY "${_opendds_try_compile_dir}")

function(_opendds_try_compile compiled_var output_var name source)
  set(no_value_options)
  set(single_value_options OUTPUT_VAR)
  set(multi_value_options COMPILE_DEFINITIONS CMAKE_FLAGS)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  unset(_opendds_try_compile_compiled CACHE)
  try_compile(_opendds_try_compile_compiled
    "${_opendds_try_compile_dir}/${name}"
    SOURCES "${source}"
    COMPILE_DEFINITIONS ${arg_COMPILE_DEFINITIONS}
    CMAKE_FLAGS ${arg_CMAKE_FLAGS}
    OUTPUT_VARIABLE output
  )
  set(${compiled_var} "${_opendds_try_compile_compiled}" PARENT_SCOPE)
  set(${output_var} "${output}" PARENT_SCOPE)
endfunction()

set(_opendds_test_cxx_std "${CMAKE_CURRENT_LIST_DIR}/test_cxx_std.cpp")

function(_opendds_try_compile_cplusplus compiled_var output_var cplusplus)
  _opendds_try_compile(compiled output
    "cplusplus_${cplusplus}" "${_opendds_test_cxx_std}"
    COMPILE_DEFINITIONS "-DOPENDDS_TEST_CPLUSPLUS=${cplusplus}L")
  set(${compiled_var} "${compiled}" PARENT_SCOPE)
  set(${output_var} "${output}" PARENT_SCOPE)
endfunction()

function(_opendds_set_cxx_std)
  set(min_cplusplus_value 199711)
  _opendds_cplusplus_to_year(default_cxx_std_year ${min_cplusplus_value})
  set(cplusplus_values 201103 201402 201703 202002 202302)
  set(invalid_cplusplus_value 999999)
  _opendds_cplusplus_to_year(invalid_cxx_std_year ${invalid_cplusplus_value})
  # (Assuming we're not running in the year 10,000 and there's still C++ standards coming out)
  set(compiler_info "compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

  # Make sure try_compile works correctly
  _opendds_try_compile_cplusplus(compiled output ${min_cplusplus_value})
  if(NOT compiled)
    message(FATAL_ERROR
      " try_compile using ${compiler_info} failed for C++ ${default_cxx_std_year}:\n"
      " \n"
      " ${output}")
  endif()
  _opendds_try_compile_cplusplus(compiled output ${invalid_cplusplus_value})
  if(compiled)
    message(FATAL_ERROR
      "try_compile using ${compiler_info} shouldn't have worked for C++ ${invalid_cxx_std_year}")
  endif()

  # Get the latest known default compiler C++ standard
  foreach(cplusplus IN LISTS cplusplus_values)
    _opendds_try_compile_cplusplus(compiled output ${cplusplus})
    if(compiled)
      _opendds_cplusplus_to_year(default_cxx_std_year ${cplusplus})
    else()
      break()
    endif()
  endforeach()
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "default_cxx_std_year: ${default_cxx_std_year}")
  endif()

  # Get the max C++ standard supported by the compiler and/or CMake
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
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "max_cxx_std_year: ${max_cxx_std_year}")
  endif()

  # Get the min C++ standard that might be used
  set(explicit TRUE)
  if(DEFINED OPENDDS_CXX_STD)
    # User is overriding the standard for OpenDDS specifically
    _opendds_cxx_std_to_year(cxx_std_year ${OPENDDS_CXX_STD})
    set(cxx_std_year_source "OPENDDS_CXX_STD")
  elseif(DEFINED CMAKE_CXX_STANDARD)
    # User is overriding the standard globally
    _opendds_cxx_std_to_year(cxx_std_year ${CMAKE_CXX_STANDARD})
    set(cxx_std_year_source "CMAKE_CXX_STANDARD")
  else()
    set(explicit FALSE)
    set(cxx_std_year ${default_cxx_std_year})
    set(cxx_std_year_source "compiler default")
  endif()
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "cxx_std_year: ${cxx_std_year} (${cxx_std_year_source})")
  endif()

  # Get the C++ standard ACE requires
  set(ace_info "ACE ${OPENDDS_ACE_VERSION}")
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
      set(include_dir "${_opendds_try_compile_dir}/ace_cxx_std_include")
      list(APPEND includes "${include_dir}")
      set(ace_dir "${include_dir}/ace")
      file(MAKE_DIRECTORY "${ace_dir}")
      file(WRITE "${ace_dir}/config.h" "#include <ace/${_OPENDDS_ACE_CONFIG_FILE}>\n")
    endif()
    foreach(try_cplusplus IN LISTS cplusplus_values)
      _opendds_cplusplus_to_year(check_year ${try_cplusplus})
      if(check_year GREATER ${max_cxx_std_year})
        message(STATUS "Had to stop probing ACE C++ standard, C++ ${check_year} is greater than "
          "the max supported C++ ${max_cxx_std_year}. ACE might require a higher standard than "
          "what is supported by the compiler and/or CMake.")
        break()
      endif()
      set(try_year ${check_year})
      _opendds_cxx_std_from_year(try_std ${try_year})
      string(REPLACE ";" "\\\;" includes "${includes}") # Needed because this will be a nested list
      _opendds_try_compile(compiled build_output
        "ace_cxx_std_${try_cplusplus}" "${_opendds_test_cxx_std}"
        COMPILE_DEFINITIONS "-DOPENDDS_TEST_ACE_CXX_STD"
        CMAKE_FLAGS
          "-DCMAKE_CXX_STANDARD=${try_std}"
          "-DINCLUDE_DIRECTORIES=${includes}"
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
      "requirement to that.")
    set(cxx_std_year ${ace_min_cxx_std_year})
    set(cxx_std_year_source "ACE")
  endif()

  _opendds_cxx_std_from_year(cxx_std ${cxx_std_year})
  unset(OPENDDS_CXX_STD PARENT_SCOPE)
  set(OPENDDS_CXX_STD ${cxx_std} CACHE STRING
    "Minimum required C++ standard (same values as CMAKE_CXX_STANDARD)" FORCE)
  message(STATUS "OPENDDS_CXX_STD: ${OPENDDS_CXX_STD} (from ${cxx_std_year_source})")
  set(OPENDDS_CXX_STD_YEAR ${cxx_std_year} CACHE STRING
    "Minimum required C++ standard year (do not set manually)" FORCE)
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
  if(OPENDDS_CMAKE_VERBOSE)
    message(STATUS "  ${name}=${value}")
  endif()

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
if(OPENDDS_CMAKE_VERBOSE)
  message(STATUS "All OpenDDS Features:")
endif()
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
if(OPENDDS_CXX_STD_YEAR LESS 2017)
  set(_opendds_cxx17 OFF)
else()
  set(_opendds_cxx17 ON)
endif()
_opendds_feature(CXX17 "${_opendds_cxx17}" MPC_INVERTED_NAME no_cxx17 DOC "Build assumes C++17 support")
_opendds_feature(STD_OPTIONAL "${OPENDDS_CXX17}" CONFIG DOC "Use C++17's std::optional")

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
set(OPENDDS_DEFAULT_GENERATED_FOLDER "IDL" CACHE STRING
  "Default value used for the FOLDER CMake property of generated targets added in opendds_target_sources")

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

function(_opendds_find_xerces_for_ace)
  # ACE needs the root of Xerces. find_package doesn't seems like it can
  # provide it, so need to extract the root from XercesC_INCLUDE_DIR or
  # XercesC_INCLUDE_DIRS and XercesC::XercesC.
  get_target_property(xerces_path XercesC::XercesC LOCATION)
  foreach(include IN LISTS XercesC_INCLUDE_DIR XercesC_INCLUDE_DIRS)
    if(include AND EXISTS "${include}")
      file(TO_CMAKE_PATH "${xerces_path}" path)
      while(TRUE)
        get_filename_component(parent "${path}" DIRECTORY)
        if(parent STREQUAL path)
          break()
        endif()
        set(path "${parent}")
        file(RELATIVE_PATH rel "${path}" "${include}")
        if(NOT rel MATCHES "^\\.\\.")
          set(_OPENDDS_XERCES3_FOR_ACE "${path}" CACHE PATH "" FORCE)
          break()
        endif()
      endwhile()
    endif()
    if(DEFINED _OPENDDS_XERCES3_FOR_ACE)
      break()
    endif()
  endforeach()
  if(NOT DEFINED _OPENDDS_XERCES3_FOR_ACE)
    message(FATAL_ERROR "Failed to extract Xerces root from: "
      "${xerces_path} AND ${XercesC_INCLUDE_DIR} AND ${XercesC_INCLUDE_DIRS}")
  endif()
endfunction()

# This should be in ace_group.cmake, but it's needed by build_ace_tao.cmake.
if(OPENDDS_XERCES3)
  find_package(XercesC PATHS "${OPENDDS_XERCES3}" NO_DEFAULT_PATH QUIET)
  if(NOT XercesC_FOUND)
    find_package(XercesC QUIET)
  endif()
  if(NOT XercesC_FOUND)
    message(FATAL_ERROR "Could not find XercesC")
  endif()

  if(ACE_IS_BEING_BUILT)
    unset(_OPENDDS_XERCES3_FOR_ACE CACHE)
    if(EXISTS "${OPENDDS_XERCES3}")
      set(_OPENDDS_XERCES3_FOR_ACE "${OPENDDS_XERCES3}" CACHE PATH "" FORCE)
    else()
      _opendds_find_xerces_for_ace()
    endif()
  endif()
endif()

set(_OPENDDS_INIT_CMAKE_COMPLETE TRUE)
