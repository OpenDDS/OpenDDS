include(ExternalProject)
include(FetchContent)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  cmake_policy(SET CMP0135 NEW)
endif()

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
  elseif(CMAKE_SYSTEM_NAME STREQUAL Android)
    set(config_file "config-android.h")
    list(APPEND configure_ace_tao_args --platform-macros-file "platform_android.GNU")
    if(NOT DEFINED ANDROID_ABI)
      message(FATAL_ERROR "ANDROID_ABI not defined!")
    endif()
    list(APPEND configure_ace_tao_args --macro-line "android_abi:=${ANDROID_ABI}")
    if(NOT DEFINED ANDROID_PLATFORM)
      message(FATAL_ERROR "ANDROID_PLATFORM not defined!")
    elseif(ANDROID_PLATFORM MATCHES "(android-)?([0-9]+)")
      list(APPEND configure_ace_tao_args --macro-line "android_api:=${CMAKE_MATCH_2}")
    else()
      message(FATAL_ERROR "ANDROID_PLATFORM is in unexpected format: ${ANDROID_PLATFORM}")
    endif()
    if(NOT DEFINED ANDROID_NDK)
      message(FATAL_ERROR "ANDROID_NDK not defined!")
    endif()
    list(APPEND configure_ace_tao_args --macro-line "android_ndk:=${ANDROID_NDK}")
    if(NOT DEFINED OPENDDS_ACE_TAO_HOST_TOOLS)
      if(DEFINED OPENDDS_HOST_TOOLS)
        if(IS_DIRECTORY "${OPENDDS_HOST_TOOLS}/ace_tao/bin")
          set(OPENDDS_ACE_TAO_HOST_TOOLS "${OPENDDS_HOST_TOOLS}/ace_tao" CACHE INTERNAL "")
        else()
          set(OPENDDS_ACE_TAO_HOST_TOOLS "${OPENDDS_HOST_TOOLS}" CACHE INTERNAL "")
        endif()
      else()
        message(FATAL_ERROR "OPENDDS_ACE_TAO_HOST_TOOLS or OPENDDS_HOST_TOOLS need to be defined!")
      endif()
    endif()
    list(APPEND configure_ace_tao_args
      --macro-line "TAO_IDL:=${OPENDDS_ACE_TAO_HOST_TOOLS}/bin/tao_idl"
      --macro-line "TAO_IDLFLAG+=-g ${OPENDDS_ACE_TAO_HOST_TOOLS}/bin/ace_gperf"
      --macro-line "TAO_IDL_DEP:=${OPENDDS_ACE_TAO_HOST_TOOLS}/bin/tao_idl"
    )
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
  set(OPENDDS_ACE_TAO_SRC "${OPENDDS_BUILD_DIR}/ace_tao")
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

if(OPENDDS_JUST_BUILD_HOST_TOOLS)
  set(ws "${OPENDDS_BUILD_DIR}/host-tools.mwc")
  file(WRITE "${ws}"
    "workspace {\n"
    "  $(ACE_ROOT)/ace/ace.mpc\n"
    "  $(ACE_ROOT)/apps/gperf/src\n"
    "  $(TAO_ROOT)/TAO_IDL\n"
    "}\n"
  )
  list(APPEND configure_ace_tao_args "--workspace-file=${ws}")
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

  set(make_cmd "${CMAKE_COMMAND}" -E env "ACE_ROOT=${OPENDDS_ACE}" "TAO_ROOT=${OPENDDS_TAO}")
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    list(APPEND make_cmd "$(MAKE)")
  else()
    find_program(make_exe NAMES gmake make)
    cmake_host_system_information(RESULT core_count QUERY NUMBER_OF_LOGICAL_CORES)
    list(APPEND make_cmd "${make_exe}" "-j${core_count}")
  endif()

  ExternalProject_Add(build_ace_tao
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "Already configured"
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND ${make_cmd}
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
