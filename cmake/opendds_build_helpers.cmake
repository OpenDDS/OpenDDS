if(_OPENDDS_BUILD_HELPERS_CMAKE)
  return()
endif()
set(_OPENDDS_BUILD_HELPERS_CMAKE TRUE)

include(GNUInstallDirs)

include("${CMAKE_CURRENT_LIST_DIR}/opendds_utils.cmake")

set(_opendds_exec_perms
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)

# Load in build settings.
_opendds_read_ini("${OPENDDS_SOURCE_DIR}/build.ini" PREFIX _OPENDDS_BUILD)

# Common settings for all targets. Shouldn't be used directly.
function(_opendds_target target scope)
  # This is the name the target should be exported from CMake-build OpenDDS as
  # or imported in the MPC-built OpenDDS CMake package. For consistency and
  # convenience, alias it so we can use it within the OpenDDS CMake build.
  string(REPLACE "OpenDDS_" "" name "${target}")
  set(name "OpenDDS::${name}")
  get_target_property(target_type ${target} TYPE)
  if(target_type MATCHES "LIBRARY")
    add_library("${name}" ALIAS "${target}")
  else()
    add_executable("${name}" ALIAS "${target}")
  endif()
  set_target_properties(${target} PROPERTIES EXPORT_NAME "${name}")

  if(OPENDDS_COMPILE_WARNINGS STREQUAL "WARNING" OR OPENDDS_COMPILE_WARNINGS STREQUAL "ERROR")
    set(prefix "_OPENDDS_BUILD_${CMAKE_CXX_COMPILER_ID}_")
    string(REPLACE " " ";" outvar "${${prefix}warning}")
    target_compile_options(${target} PRIVATE ${outvar})
    if(OPENDDS_COMPILE_WARNINGS STREQUAL "ERROR")
      string(REPLACE " " ";" outvar "${${prefix}error}")
      target_compile_options(${target} PRIVATE ${outvar})
    endif()
  endif()

  _opendds_cxx_std(${target} ${scope})

  if(NOT target STREQUAL OpenDDS_Config)
    target_link_libraries(${target} ${scope} OpenDDS_Config)
  endif()
endfunction()

# Helper for _opendds_include
function(_opendds_include_once target prop target_scope include)
  get_target_property(current ${target} ${prop})
  if(NOT current)
    set(current "")
  endif()

  if(NOT include IN_LIST current)
    target_include_directories(${target} ${target_scope} "${include}")
  endif()
endfunction()

# Alternative to target_include_directories that doesn't add the include if
# it's already included.
function(_opendds_include target scope include)
  if(scope STREQUAL "PRIVATE" OR scope STREQUAL "PUBLIC")
    _opendds_include_once(${target} INCLUDE_DIRECTORIES PRIVATE "${include}")
  endif()

  if(scope STREQUAL "INTERFACE" OR scope STREQUAL "PUBLIC")
    _opendds_include_once(${target} INTERFACE_INCLUDE_DIRECTORIES INTERFACE "${include}")
  endif()
endfunction()

# Helper for _opendds_headers
function(_opendds_headers_file_set target scope base_dir headers install_headers)
  if(headers)
    get_target_property(target_type ${target} TYPE)
    set(is_interface FALSE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
      set(is_interface TRUE)
    endif()

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
      target_sources(${target}
        ${scope} FILE_SET HEADERS
        BASE_DIRS "${base_dir}"
        FILES ${headers}
      )
    else() # If <3.23, emulate file sets in _opendds_library

      # Non-PRIVATE headers should be installed
      if(scope STREQUAL "PUBLIC" OR scope STREQUAL "INTERFACE")
        list(APPEND install_headers ${headers})
      endif()

      # Make the headers visible in IDEs, needs to be PRIVATE, but this doesn't
      # work before 3.19 for interface libraries like OpenDDS_Config.
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19 OR NOT is_interface)
        target_sources(${target} PRIVATE ${headers})
        # Make sure CMake doesn't try to compile the "_T.cpp" headers
        foreach(header IN LISTS headers)
          set_source_files_properties("${header}" PROPERTIES HEADER_FILE_ONLY TRUE)
        endforeach()
      endif()

      # Emulate FILE_SET [INTERFACE_]INCLUDE_DIRECTORY behavior, except don't
      # repeat the include if it already is included.
      _opendds_include(${target} ${scope} "$<BUILD_INTERFACE:${base_dir}>")
    endif()
    set(install_headers "${install_headers}" PARENT_SCOPE)
  endif()
endfunction()

# Add headers to a target, separated by scope. This is calls or emulates
# target_sources(... FILE_SET HEADERS ...).
# - BASE_DIR <dir>
#   Relative base of the headers when installed (what would include be?).
# - PRIVATE <header>...
#   Headers of the target only used by the target, not installed.
# - INTERFACE <header>...
#   Headers of the target only used by users of the target, installed.
# - PUBLIC <header>...
#   Headers of the target used by everyone, installed.
function(_opendds_headers target)
  set(no_value_options)
  set(single_value_options BASE_DIR)
  set(multi_value_options PUBLIC INTERFACE PRIVATE)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  if(NOT arg_BASE_DIR)
    set(arg_BASE_DIR "${OPENDDS_SOURCE_DIR}")
  endif()

  set(install_headers)
  _opendds_headers_file_set(${target} PRIVATE "${arg_BASE_DIR}" "${arg_PRIVATE}" "${install_headers}")
  _opendds_headers_file_set(${target} INTERFACE "${arg_BASE_DIR}" "${arg_INTERFACE}" "${install_headers}")
  _opendds_headers_file_set(${target} PUBLIC "${arg_BASE_DIR}" "${arg_PUBLIC}" "${install_headers}")
  set_target_properties(${target} PROPERTIES
    _OPENDDS_INSTALL_HEADERS "${install_headers}"
    _OPENDDS_INSTALL_HEADERS_BASE_DIR "${arg_BASE_DIR}"
  )
endfunction()

# Common settings for all library targets.
# - BIGOBJ
#   Passes /bigobj to msvc on Windows
# - NO_INSTALL
#   Don't install the library or the headers for it
# - EXPORT_MACRO_PREFIX <name>
#   Override the prefix used in export macro
function(_opendds_library target)
  set(no_value_options BIGOBJ NO_INSTALL)
  set(single_value_options EXPORT_MACRO_PREFIX)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  get_target_property(target_type ${target} TYPE)
  set(lib_scope PUBLIC)
  set(is_interface FALSE)
  if(target_type STREQUAL "INTERFACE_LIBRARY")
    set(lib_scope INTERFACE)
    set(is_interface TRUE)
  endif()

  _opendds_target(${target} ${lib_scope})

  if(NOT is_interface)
    # Put library in BINARY_DIR/lib
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
      LIBRARY_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
      ARCHIVE_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
      POSITION_INDEPENDENT_CODE TRUE
      BUILD_RPATH "${OPENDDS_LIB_DIR}"
    )

    # Set up export header
    set(export_args)
    if(DEFINED arg_EXPORT_MACRO_PREFIX)
      list(APPEND export_args MACRO_PREFIX "${arg_EXPORT_MACRO_PREFIX}")
    endif()
    opendds_export_header(${target} EXISTING ${export_args})

    # Set Windows /bigobj
    if(arg_BIGOBJ)
      opendds_bigobj(${target})
    endif()
  endif()

  if(NOT arg_NO_INSTALL)
    # Install headers
    set(install_args)
    set(include_dir "${CMAKE_INSTALL_INCLUDEDIR}")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
      set(install_args FILE_SET HEADERS DESTINATION "${include_dir}")
    else()
      get_target_property(headers ${target} _OPENDDS_INSTALL_HEADERS)
      if(headers)
        get_target_property(base_dir ${target} _OPENDDS_INSTALL_HEADERS_BASE_DIR)
        foreach(header IN LISTS headers)
          get_filename_component(header_abs "${header}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
          file(RELATIVE_PATH header_rel "${base_dir}" "${header_abs}")
          get_filename_component(header_dir "${header_rel}" DIRECTORY)

          install(FILES "${header}" DESTINATION "${include_dir}/${header_dir}")
        endforeach()
      endif()
    endif()

    # Make sure install interface include is set
    # The build interface is guaranteed by the header file set.
    _opendds_include(${target} ${lib_scope} "$<INSTALL_INTERFACE:${include_dir}>")

    # Install everything
    install(TARGETS ${target}
      EXPORT opendds_targets
      LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS ${_opendds_exec_perms}
      RUNTIME
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS ${_opendds_exec_perms}
      ARCHIVE DESTINATION lib
      ${install_args}
    )
  endif()
endfunction()

# Common settings for executable library targets.
# - NO_INSTALL
#   Don't install the executable.
function(_opendds_executable target)
  set(no_value_options NO_INSTALL)
  set(single_value_options)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  _opendds_target(${target} PRIVATE)

  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OPENDDS_BIN_DIR}")

  if(NOT arg_NO_INSTALL)
    install(TARGETS ${target}
      EXPORT opendds_targets
      RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS ${_opendds_exec_perms}
    )
  endif()
endfunction()
