include(ExternalProject)

set(_opendds_std_target OpenDDS_Dcps)
if(OPENDDS_JUST_BUILD_HOST_TOOLS)
  set(ws "${OPENDDS_BUILD_DIR}/host-tools.mwc")
  file(WRITE "${ws}"
    "workspace {\n"
    "  $(ACE_ROOT)/ace/ace.mpc\n"
    "  $(ACE_ROOT)/apps/gperf/src\n"
    "  $(TAO_ROOT)/TAO_IDL\n"
    "}\n"
  )
  list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS "--workspace-file=${ws}")
  set(_opendds_std_target opendds_idl)
endif()

# Get the C++ standard OpenDDS is going to be built with. We are going to force
# the ACE/TAO build to use the same standard.
set(_opendds_std "$<TARGET_PROPERTY:${_opendds_std_target},CXX_STANDARD>")

find_package(Perl REQUIRED)
if(OPENDDS_STATIC)
  list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --static=1)
endif()
list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS
  --compiler "${CMAKE_CXX_COMPILER}"
)
execute_process(
  COMMAND
    "${PERL_EXECUTABLE}" "${_OPENDDS_CMAKE_DIR}/configure_ace_tao.pl"
    --mpc "${OPENDDS_MPC}"
    --mpc-type "${_OPENDDS_MPC_TYPE}"
    --src "${OPENDDS_ACE_TAO_SRC}"
    --ace "${OPENDDS_ACE}"
    --tao "${OPENDDS_TAO}"
    ${_OPENDDS_CONFIGURE_ACE_TAO_ARGS}
    --config-file "${_OPENDDS_ACE_CONFIG_FILE}"
    ${_OPENDDS_MPC_FEATURES}
  COMMAND_ECHO STDOUT
  COMMAND_ERROR_IS_FATAL ANY
)

set(_build_cmd "${CMAKE_COMMAND}" -E env "ACE_ROOT=${OPENDDS_ACE}" "TAO_ROOT=${OPENDDS_TAO}")
if(_OPENDDS_XERCES3_FOR_ACE)
  list(APPEND _build_cmd "XERCESCROOT=${_OPENDDS_XERCES3_FOR_ACE}")
endif()

if(_OPENDDS_MPC_TYPE STREQUAL gnuace)
  execute_process(
    COMMAND
      "${PERL_EXECUTABLE}" "${_OPENDDS_CMAKE_DIR}/scrape_gnuace.pl"
      --workspace "${OPENDDS_ACE_TAO_SRC}"
      --loc-base "${OPENDDS_BUILD_DIR}"
      --ace "${OPENDDS_ACE}"
      --tao "${OPENDDS_TAO}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_VARIABLE mpc_projects
  )
  set(_OPENDDS_ACE_MPC_PROJECTS "${mpc_projects}" CACHE INTERNAL "")
  set(_OPENDDS_ACE_MPC_EXTERNAL_PROJECT "build_ace_tao" CACHE INTERNAL "")
  set(_OPENDDS_TAO_MPC_PROJECTS "${mpc_projects}" CACHE INTERNAL "")
  set(_OPENDDS_TAO_MPC_EXTERNAL_PROJECT "build_ace_tao" CACHE INTERNAL "")

  string(JSON project_count LENGTH "${mpc_projects}")
  if(project_count EQUAL 0)
    message(FATAL_ERROR "MPC projects was empty!")
  endif()
  set(byproducts)
  math(EXPR member_index_end "${project_count} - 1")
  foreach(member_index RANGE ${member_index_end})
    string(JSON member_name MEMBER "${mpc_projects}" ${member_index})
    string(JSON mpc_project GET "${mpc_projects}" ${member_name})
    string(JSON file GET "${mpc_project}" loc)
    list(APPEND byproducts "${file}")
  endforeach()

  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    list(APPEND _build_cmd "$(MAKE)")
  else()
    find_program(make_exe NAMES gmake make)
    cmake_host_system_information(RESULT core_count QUERY NUMBER_OF_LOGICAL_CORES)
    list(APPEND _build_cmd "${make_exe}" "-j${core_count}")
  endif()

  list(APPEND _build_cmd
    "opendds_cmake_std=$<IF:$<BOOL:${_opendds_std}>,-std=c++${_opendds_std},>"
  )

  ExternalProject_Add(build_ace_tao
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "Already configured"
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND ${_build_cmd}
    BUILD_BYPRODUCTS ${byproducts}
    USES_TERMINAL_BUILD TRUE # Needed for Ninja to show the ACE/TAO build
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "No install step"
  )
elseif(_OPENDDS_MPC_TYPE MATCHES "^vs")
  set(sln ACE_TAO_for_OpenDDS.sln)
  execute_process(
    COMMAND
      "${PERL_EXECUTABLE}" "${_OPENDDS_CMAKE_DIR}/scrape_vs.pl"
      --sln "${OPENDDS_ACE_TAO_SRC}/${sln}"
      --ace "${OPENDDS_ACE}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_VARIABLE mpc_projects
  )
  set(_OPENDDS_ACE_MPC_PROJECTS "${mpc_projects}" CACHE INTERNAL "")
  set(_OPENDDS_ACE_MPC_EXTERNAL_PROJECT "build_ace_tao" CACHE INTERNAL "")
  set(_OPENDDS_TAO_MPC_PROJECTS "${mpc_projects}" CACHE INTERNAL "")
  set(_OPENDDS_TAO_MPC_EXTERNAL_PROJECT "build_ace_tao" CACHE INTERNAL "")

  string(JSON project_count LENGTH "${mpc_projects}")
  if(project_count EQUAL 0)
    message(FATAL_ERROR "MPC projects was empty!")
  endif()
  set(byproducts)
  math(EXPR member_index_end "${project_count} - 1")
  foreach(member_index RANGE ${member_index_end})
    string(JSON member_name MEMBER "${mpc_projects}" ${member_index})
    string(JSON mpc_project GET "${mpc_projects}" ${member_name})
    string(JSON configs ERROR_VARIABLE err GET "${mpc_project}" "configs")
    if(configs)
      string(JSON config_index_end LENGTH "${configs}")
      math(EXPR config_index_end "${config_index_end} - 1")
      foreach(config_index RANGE ${config_index_end})
        string(JSON config_name MEMBER "${configs}" ${config_index})
        string(JSON path GET "${configs}" ${config_name})
        list(APPEND byproducts "${path}")
      endforeach()
    else()
      string(JSON path GET "${mpc_project}" "loc")
      list(APPEND byproducts "${path}")
    endif()
  endforeach()

  list(APPEND _build_cmd
    MSBuild "${sln}" "-maxcpucount"
    "-property:Configuration=$<CONFIG>,Platform=${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}"
    "$<IF:$<BOOL:${_opendds_std}>,-property:LanguageStandard=stdcpp${_opendds_std},>"
  )

  ExternalProject_Add(build_ace_tao
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "Already configured"
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND ${_build_cmd}
    BUILD_BYPRODUCTS ${byproducts}
    USES_TERMINAL_BUILD TRUE # Needed for Ninja to show the ACE/TAO build
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "No install step"
  )
else()
  message(FATAL_ERROR "Not sure how to build ACE/TAO for this system "
    "(${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR}/${_OPENDDS_MPC_TYPE})")
endif()
