include(ExternalProject)
include(FetchContent)

set(ACE_TAO_CONFIGURED FALSE CACHE INTERNAL "")
set(ACE_IS_BEING_BUILT MPC CACHE INTERNAL "")
set(TAO_IS_BEING_BUILT MPC CACHE INTERNAL "")
set(url_base "https://github.com/DOCGroup/ACE_TAO/releases/download/")
set(ace_ver "7.1.1")
set(configure_ace_tao_args)
if(WIN32)
  set(ext "zip")
  set(md5 "40d99f613047a15665c205dacf2e066e")
  if(CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022")
    set(mpc_type vs2022)
  elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 16 2019")
    set(mpc_type vs2019)
  elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 15 2017")
    set(mpc_type vs2017)
  elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015")
    set(mpc_type vc14)
  else()
    set(mpc_type vs2022)
  endif()
  set(config_file "config-win32.h")
else()
  set(ext "tar.bz2")
  set(md5 "2f60399b059dfd184c8248443920b6e5")
  set(mpc_type gnuace)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(config_file "config-linux.h")
    list(APPEND configure_ace_tao_args --platform-macros-file "platform_linux.GNU")
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(config_file "config-macosx.h")
    list(APPEND configure_ace_tao_args --platform-macros-file "platform_macosx.GNU")
  endif()
endif()
if(NOT DEFINED mpc_type OR NOT DEFINED config_file)
  message(FATAL_ERROR "Not sure how to configure ACE/TAO for this system "
    "(${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR})")
endif()

string(REPLACE "." "_" ace_ver_tag "${ace_ver}")
if(DEFINED OPENDDS_ACE_TAO_SRC)
  if(NOT DEFINED OPENDDS_MPC)
    message(FATAL_ERROR "OPENDDS_ACE_TAO_SRC requires OPENDDS_MPC to be set")
  endif()
  if(EXISTS "${OPENDDS_ACE_TAO_SRC}/ace/Version.h")
    set(OPENDDS_ACE "${OPENDDS_ACE_TAO_SRC}" CACHE INTERNAL "")
  elseif(EXISTS "${OPENDDS_ACE_TAO_SRC}/ACE/ace/Version.h")
    set(OPENDDS_ACE "${OPENDDS_ACE_TAO_SRC}/ACE" CACHE INTERNAL "")
    if(mpc_type STREQUAL gnuace)
      set(_OPENDDS_ACE_MPC_NAME_IS_ACE_TARGET TRUE CACHE INTERNAL "")
    endif()
  else()
    message(FATAL_ERROR "Can't find ace/Version.h in OPENDDS_ACE_TAO_SRC")
  endif()
  if(EXISTS "${OPENDDS_ACE_TAO_SRC}/TAO/tao/Version.h")
    set(OPENDDS_TAO "${OPENDDS_ACE_TAO_SRC}/TAO" CACHE INTERNAL "")
  else()
    message(FATAL_ERROR "Can't find tao/Version.h in OPENDDS_ACE_TAO_SRC")
  endif()
else()
  set(OPENDDS_ACE_TAO_SRC "${OPENDDS_BUILD_DIR}/ace_tao" CACHE INTERNAL "")
  set(OPENDDS_ACE "${OPENDDS_ACE_TAO_SRC}" CACHE INTERNAL "")
  set(OPENDDS_MPC "${OPENDDS_ACE}/MPC" CACHE INTERNAL "")
  set(OPENDDS_TAO "${OPENDDS_ACE}/TAO" CACHE INTERNAL "")
  FetchContent_Declare(ace_tao_dl
    PREFIX "${OPENDDS_BUILD_DIR}/ace_tao_tmp"
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    URL "${url_base}ACE+TAO-${ace_ver_tag}/ACE+TAO-src-${ace_ver}.${ext}"
    URL_MD5 "${md5}"
  )
  FetchContent_Populate(ace_tao_dl)
endif()

if(mpc_type STREQUAL gnuace)
  set(_OPENDDS_TAO_MPC_NAME_IS_TAO_TARGET TRUE CACHE INTERNAL "")
endif()

find_package(Perl REQUIRED)
if(OPENDDS_STATIC)
  list(APPEND configure_ace_tao_args --static=1)
endif()
execute_process(
  COMMAND
    "${PERL_EXECUTABLE}" "${_OPENDDS_CMAKE_DIR}/configure_ace_tao.pl"
    --mpc "${OPENDDS_MPC}"
    --mpc-type "${mpc_type}"
    --src "${OPENDDS_ACE_TAO_SRC}"
    --ace "${OPENDDS_ACE}"
    --tao "${OPENDDS_TAO}"
    ${configure_ace_tao_args}
    --config-file "${config_file}"
    ${_OPENDDS_MPC_FEATURES}
  COMMAND_ECHO STDOUT
  COMMAND_ERROR_IS_FATAL ANY
)

if(mpc_type STREQUAL gnuace)
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

  ExternalProject_Add(build_ace_tao
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "Already configured"
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND
      "${CMAKE_COMMAND}" -E env "ACE_ROOT=${OPENDDS_ACE}" "TAO_ROOT=${OPENDDS_TAO}"
      make -j8
    BUILD_BYPRODUCTS ${byproducts}
    USES_TERMINAL_BUILD TRUE # Needed for Ninja to show the ACE/TAO build
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "No install step"
  )
elseif(mpc_type MATCHES "^vs")
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
    string(JSON file GET "${mpc_project}" loc)
    list(APPEND byproducts "${file}")
  endforeach()

  ExternalProject_Add(build_ace_tao
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "Already configured"
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND
      "${CMAKE_COMMAND}" -E env "ACE_ROOT=${OPENDDS_ACE}" "TAO_ROOT=${OPENDDS_TAO}"
      MSBuild "${sln}" -property:Configuration=$<CONFIG>,Platform=${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}
    BUILD_BYPRODUCTS ${byproducts}
    USES_TERMINAL_BUILD TRUE # Needed for Ninja to show the ACE/TAO build
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "No install step"
  )
else()
  message(FATAL_ERROR "Not sure how to build ACE/TAO for this system")
endif()

set(ACE_TAO_CONFIGURED TRUE CACHE INTERNAL "")
