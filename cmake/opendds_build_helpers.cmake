function(_opendds_alias target)
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
endfunction()

function(_opendds_library target)
  set(no_value_options)
  set(single_value_options EXPORT_SYMBOLS_NAME)
  set(multi_value_options)
  cmake_parse_arguments(arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}" ${ARGN})

  _opendds_alias(${target})

  # Put library in BINARY_DIR/lib
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
  )

  get_target_property(target_type ${target} TYPE)
  if(NOT DEFINED arg_EXPORT_SYMBOLS_NAME)
    set(arg_EXPORT_SYMBOLS_NAME "${target}")
  endif()
  string(TOUPPER "${arg_EXPORT_NAME}" export_symbols_name)
  if(target_type STREQUAL "SHARED_LIBRARY")
    # Define macro for export header
    target_compile_definitions(${target} PRIVATE "${export_symbols_name}_BUILD_DLL")
  elseif(target_type STREQUAL "STATIC_LIBRARY")
    # Define macro for dds/DCPS/InitStaticLibs.h and other files
    string(REPLACE "OPENDDS_" "" short_export_symbols_name "${export_symbols_name}")
    target_compile_definitions(${target} PUBLIC "OPENDDS_${short_export_symbols_name}_HAS_DLL=0")
  else()
    message(FATAL_ERROR "Target ${target} has unexpected type ${target_type}")
  endif()

  set(exec_perms
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)
  # TODO: Export Targets
  install(TARGETS ${target}
    LIBRARY
      DESTINATION lib
      PERMISSIONS ${exec_perms}
    RUNTIME
      DESTINATION lib
      PERMISSIONS ${exec_perms}
    ARCHIVE DESTINATION lib
  )
endfunction()

function(_opendds_executable target)
  _opendds_alias(${target})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OPENDDS_BIN_DIR}")

  set(exec_perms
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)
  install(TARGETS ${target}
    RUNTIME
      DESTINATION bin
      PERMISSIONS ${exec_perms}
  )
endfunction()
