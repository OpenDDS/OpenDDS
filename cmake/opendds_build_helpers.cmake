function(_opendds_alias target)
  # This is the name the target should be exported from CMake-build OpenDDS as
  # or imported in the MPC-built OpenDDS CMake package. For consistency and
  # convenience, alias it so we can use it within the OpenDDS CMake build.
  string(REPLACE "OpenDDS_" "" user_name "${target}")
  set(user_name "OpenDDS::${user_name}")
  get_target_property(target_type ${target} TYPE)
  if(target_type MATCHES "LIBRARY")
    add_library("${user_name}" ALIAS "${target}")
  else()
    add_executable("${user_name}" ALIAS "${target}")
  endif()
endfunction()

function(_opendds_library target)
  _opendds_alias(${target})

  # Put library in BINARY_DIR/lib
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OPENDDS_LIB_DIR}"
  )

  get_target_property(target_type ${target} TYPE)
  string(TOUPPER "${target}" uppercase_target)
  if(target_type STREQUAL "SHARED_LIBRARY")
    # Define macro for export header
    target_compile_definitions(${target} PRIVATE "${uppercase_target}_BUILD_DLL")
  elseif(target_type STREQUAL "STATIC_LIBRARY")
    # Define macro for dds/DCPS/InitStaticLibs.h and other files
    string(REPLACE "OPENDDS_" "" short_uppercase_target "${uppercase_target}")
    target_compile_definitions(${target} PUBLIC "OPENDDS_${short_uppercase_target}_HAS_DLL=0")
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
