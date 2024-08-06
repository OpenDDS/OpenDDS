include(FetchContent)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  cmake_policy(SET CMP0135 NEW)
endif()

function(_opendds_set_vs_mpc_type)
  set(vs_vers
    "Visual Studio 14 2015" vc14
    "Visual Studio 15 2017" vs2017
    "Visual Studio 16 2019" vs2019
    "Visual Studio 17 2022" vs2022
  )

  set(mpc_vs_name)
  list(LENGTH vs_vers vs_vers_count)
  math(EXPR vs_vers_count_end "${vs_vers_count} - 1")
  foreach(i RANGE 0 ${vs_vers_count_end} 2)
    list(GET vs_vers ${i} cmake_name)
    math(EXPR next "${i} + 1")
    list(GET vs_vers ${next} mpc_name)
    if(cmake_name STREQUAL CMAKE_GENERATOR)
      set(mpc_vs_name "${mpc_name}")
      break()
    endif()
  endforeach()
  if(NOT mpc_vs_name)
    message(STATUS "Missing a MPC project type for \"${CMAKE_GENERATOR}\", "
      "using last known: \"${cmake_name}\"/${mpc_name}")
    set(mpc_vs_name "${mpc_name}")
  endif()
  set(_OPENDDS_MPC_TYPE "${mpc_vs_name}" PARENT_SCOPE)
endfunction()

set(_OPENDDS_CONFIGURE_ACE_TAO_ARGS)
set(ACE_IS_BEING_BUILT MPC CACHE INTERNAL "")
set(TAO_IS_BEING_BUILT MPC CACHE INTERNAL "")
if(WIN32)
  set(ext "zip")
  _opendds_set_vs_mpc_type()
  set(_OPENDDS_ACE_CONFIG_FILE "config-win32.h")
else()
  set(ext "tar.bz2")
  set(_OPENDDS_MPC_TYPE gnuace)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(_OPENDDS_ACE_CONFIG_FILE "config-linux.h")
    list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --platform-macros-file "platform_linux.GNU")
  elseif(CMAKE_SYSTEM_NAME STREQUAL Android)
    set(_OPENDDS_ACE_CONFIG_FILE "config-android.h")
    list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --platform-macros-file "platform_android.GNU")
    if(NOT DEFINED ANDROID_ABI)
      message(FATAL_ERROR "ANDROID_ABI not defined!")
    endif()
    list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --macro-line "android_abi:=${ANDROID_ABI}")
    if(NOT DEFINED ANDROID_PLATFORM)
      message(FATAL_ERROR "ANDROID_PLATFORM not defined!")
    elseif(ANDROID_PLATFORM MATCHES "(android-)?([0-9]+)")
      list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --macro-line "android_api:=${CMAKE_MATCH_2}")
    else()
      message(FATAL_ERROR "ANDROID_PLATFORM is in unexpected format: ${ANDROID_PLATFORM}")
    endif()
    if(NOT DEFINED ANDROID_NDK)
      message(FATAL_ERROR "ANDROID_NDK not defined!")
    endif()
    list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --macro-line "android_ndk:=${ANDROID_NDK}")
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
    list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS
      --macro-line "TAO_IDL:=${OPENDDS_ACE_TAO_HOST_TOOLS}/bin/tao_idl"
      --macro-line "TAO_IDLFLAG+=-g ${OPENDDS_ACE_TAO_HOST_TOOLS}/bin/ace_gperf"
      --macro-line "TAO_IDL_DEP:=${OPENDDS_ACE_TAO_HOST_TOOLS}/bin/tao_idl"
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(_OPENDDS_ACE_CONFIG_FILE "config-macosx.h")
    list(APPEND _OPENDDS_CONFIGURE_ACE_TAO_ARGS --platform-macros-file "platform_macosx.GNU")
  endif()
endif()
if(NOT DEFINED _OPENDDS_MPC_TYPE OR NOT DEFINED _OPENDDS_ACE_CONFIG_FILE)
  message(FATAL_ERROR "Not sure how to configure ACE/TAO for this system "
    "(${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR})")
endif()

if(NOT DEFINED OPENDDS_ACE_TAO_SRC)
  set(OPENDDS_ACE_TAO_SRC "${OPENDDS_BUILD_DIR}/ace_tao")

  file(STRINGS "${OPENDDS_SOURCE_DIR}/acetao.ini" ace_tao_ini)
  unset(section)
  foreach(line IN LISTS ace_tao_ini)
    if(line MATCHES "^\\[(.*)\\]$")
      set(section "${CMAKE_MATCH_1}")
    elseif(section AND line MATCHES "^([^=#]+)=(.*)$")
      set(name "${CMAKE_MATCH_1}")
      set(value "${CMAKE_MATCH_2}")
      set("${section}-${name}" "${value}")
    endif()
  endforeach()

  if(NOT DEFINED OPENDDS_ACE_TAO_KIND)
    set(OPENDDS_ACE_TAO_KIND ace7tao3)
  endif()
  set(url "${${OPENDDS_ACE_TAO_KIND}-${ext}-url}")
  set(md5 "${${OPENDDS_ACE_TAO_KIND}-${ext}-md5}")
  set(repo "${${OPENDDS_ACE_TAO_KIND}-repo}")
  set(branch "${${OPENDDS_ACE_TAO_KIND}-branch}")

  if(OPENDDS_ACE_TAO_GIT OR DEFINED OPENDDS_ACE_TAO_GIT_REPO OR DEFINED OPENDDS_ACE_TAO_GIT_TAG)
    if(NOT DEFINED OPENDDS_MPC)
      set(OPENDDS_MPC_GIT TRUE)
    endif()

    if(NOT DEFINED OPENDDS_ACE_TAO_GIT_REPO)
      set(OPENDDS_ACE_TAO_GIT_REPO "${repo}")
    endif()
    if(NOT DEFINED OPENDDS_ACE_TAO_GIT_TAG)
      set(OPENDDS_ACE_TAO_GIT_TAG "${branch}")
    endif()
    message(STATUS "Getting ACE/TAO from ${OPENDDS_ACE_TAO_GIT_REPO} ${OPENDDS_ACE_TAO_GIT_TAG}")
    set(fetch_args
      GIT_REPOSITORY "${OPENDDS_ACE_TAO_GIT_REPO}"
      GIT_TAG "${OPENDDS_ACE_TAO_GIT_TAG}"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
      USES_TERMINAL_DOWNLOAD TRUE
    )
  else()
    message(STATUS "Getting ACE/TAO from ${url}")
    set(fetch_args
      URL "${url}"
      URL_MD5 "${md5}"
    )
  endif()

  FetchContent_Declare(ace_tao_dl
    PREFIX "${OPENDDS_BUILD_DIR}/ace_tao_tmp"
    SOURCE_DIR "${OPENDDS_ACE_TAO_SRC}"
    ${fetch_args}
  )
  FetchContent_Populate(ace_tao_dl)
endif()

if(EXISTS "${OPENDDS_ACE_TAO_SRC}/ace/Version.h")
  set(OPENDDS_ACE "${OPENDDS_ACE_TAO_SRC}" CACHE INTERNAL "")
elseif(EXISTS "${OPENDDS_ACE_TAO_SRC}/ACE/ace/Version.h")
  set(OPENDDS_ACE "${OPENDDS_ACE_TAO_SRC}/ACE" CACHE INTERNAL "")
else()
  message(FATAL_ERROR "Can't find ace/Version.h in ${OPENDDS_ACE}")
endif()

if(EXISTS "${OPENDDS_ACE_TAO_SRC}/TAO/tao/Version.h")
  set(OPENDDS_TAO "${OPENDDS_ACE_TAO_SRC}/TAO" CACHE INTERNAL "")
else()
  message(FATAL_ERROR "Can't find tao/Version.h in ${OPENDDS_TAO}")
endif()

if(NOT DEFINED OPENDDS_MPC)
  if(EXISTS "${OPENDDS_ACE}/MPC/mpc.pl")
    set(OPENDDS_MPC "${OPENDDS_ACE}/MPC" CACHE INTERNAL "")
  elseif(OPENDDS_MPC_GIT OR DEFINED OPENDDS_MPC_GIT_REPO OR DEFINED OPENDDS_MPC_GIT_TAG)
    set(OPENDDS_MPC "${OPENDDS_BUILD_DIR}/MPC" CACHE INTERNAL "")
    if(NOT DEFINED OPENDDS_MPC_GIT_REPO)
      set(OPENDDS_MPC_GIT_REPO "https://github.com/DOCGroup/MPC")
    endif()
    if(NOT DEFINED OPENDDS_MPC_GIT_TAG)
      set(OPENDDS_MPC_GIT_TAG "master")
    endif()
    message(STATUS "Getting MPC from ${OPENDDS_MPC_GIT_REPO} ${OPENDDS_MPC_GIT_TAG}")
    FetchContent_Declare(mpc_dl
      PREFIX "${OPENDDS_BUILD_DIR}/mpc_tmp"
      SOURCE_DIR "${OPENDDS_MPC}"
      GIT_REPOSITORY "${OPENDDS_MPC_GIT_REPO}"
      GIT_TAG "${OPENDDS_MPC_GIT_TAG}"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
      USES_TERMINAL_DOWNLOAD TRUE
    )
    FetchContent_Populate(mpc_dl)
  else()
    message(FATAL_ERROR "Can't find MPC in ${OPENDDS_ACE}, set OPENDDS_MPC or OPENDDS_MPC_GIT")
  endif()
endif()
