cmake_minimum_required(VERSION 3.14.0)
project(OpenDDS CXX)

# ******************************************************************
# Defaults Section
# ******************************************************************

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(OPENDDS_BUILT_IN_TOPICS "Built-in Topics" ON)
option(OPENDDS_CONTENT_FILTERED_TOPIC "ContentFilteredTopic (CS Profile)" ON)
option(OPENDDS_CONTENT_SUBSCRIPTION "Content-Subscription Profile" ON)
option(OPENDDS_MULTI_TOPIC "MultiTopic (CS Profile)" ON)
option(OPENDDS_OBJECT_MODEL_PROFILE "Object Model Profile" ON)
option(OPENDDS_OWNERSHIP_KIND_EXCLUSIVE "Exclusive Ownership (Ownership Profile)" ON)
option(OPENDDS_OWNERSHIP_PROFILE "Ownership Profile" ON)
option(OPENDDS_PERSISTENCE_PROFILE "Persistence Profile" ON)
option(OPENDDS_QUERY_CONDITION "QueryCondition (CS Profile)" ON)
option(OPENDDS_SUPPRESS_ANYS "Suppress CORBA::Any's" ON)
option(OPENDDS_INLINE "Inlining" ON)
option(OPENDDS_SECURITY "DDS Security Plugin" OFF)
option(OPENDDS_RAPIDJSON "RapidJSON for Wireshark dissector and JSON Sample Serialization" ON)

## OPENDDS_SAFETY_PROFILE "basis" or "extended"
## OPENDDS_XERCES3 "Set this if XercesC is not installed in a standard location"
## OPENDDS_OPENSSL "Set this if OpenSSL is not installed in a standard location"

# ******************************************************************
# External Configuration Section
# ******************************************************************

## Check for ACE_ROOT
if(NOT ACE_ROOT AND DEFINED ENV{ACE_ROOT})
  set(ACE_ROOT $ENV{ACE_ROOT})
endif()
if(ACE_ROOT)
  set(ACE_BIN_DIR ${ACE_ROOT}/bin)
  set(ACE_INCLUDE_DIRS ${ACE_ROOT})
else()
  message(FATAL_ERROR "You must provide ACE_ROOT either through -D or environment variable.")
endif()

## Check for TAO_ROOT
if(NOT TAO_ROOT AND DEFINED ENV{TAO_ROOT})
  set(TAO_ROOT $ENV{TAO_ROOT})
endif()
if(TAO_ROOT)
  set(TAO_BIN_DIR ${ACE_ROOT}/bin)
  set(TAO_LIB_DIR ${ACE_ROOT}/lib)
  set(TAO_INCLUDE_DIR ${TAO_ROOT})
  set(TAO_INCLUDE_DIRS "${TAO_INCLUDE_DIR}" "${TAO_INCLUDE_DIR}/orbsvcs")
else()
  message(FATAL_ERROR "You must provide TAO_ROOT either through -D or environment variable.")
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
  FIND_PACKAGE_ARGS NAMES GTest
)

FetchContent_Declare(
  rapidjson
  GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
  GIT_TAG master
  FIND_PACKAGE_ARGS
)

## If we are building shared libraries, build google test as shared too.
if(BUILD_SHARED_LIBS)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

if(OPENDDS_RAPIDJSON)
  ## RapidJSON contains a reference to googletest.  We need to turn off
  ## building tests in order to avoid conflicts between our version and theirs.
  set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Turn off building tests.")
  set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "We don't need the examples.")

  FetchContent_MakeAvailable(rapidjson)
endif()

# ******************************************************************
# Internal Configuration Section
# ******************************************************************

## Add our cmake directory to get internal files
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## Set up DDS_ROOT
set(DDS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENDDS_INCLUDE_DIRS ${DDS_ROOT})
set(OPENDDS_BIN_DIR ${DDS_ROOT}/bin)

if(NOT ${OPENDDS_BUILT_IN_TOPICS})
  set(OPENDDS_BUILT_IN_TOPICS_MACRO    "DDS_HAS_MINIMUM_BIT")
  set(OPENDDS_BUILT_IN_TOPICS_OPTION "-DDDS_HAS_MINIMUM_BIT")
endif()

if(OPENDDS_SAFETY_PROFILE)
  string(TOUPPER OPENDDS_SAFETY_PROFILE OPENDDS_SAFETY_PROFILE)
  set(OPENDDS_SAFETY_PROFILE_MACRO OPENDDS_SAFETY_PROFILE
      OPENDDS_NO_PERSISTENCE_PROFILE TAOLIB_ERROR=ACELIB_ERROR
      TAOLIB_DEBUG=ACELIB_DEBUG ACE_FACE_SAFETY_${OPENDDS_SAFETY_PROFILE})
  set(OPENDDS_SAFETY_PROFILE_OPTION -DOPENDDS_SAFETY_PROFILE -Sa -St)
  set(OPENDDS_SAFETY_PROFILE_TAOIDL_OPTION -SS)
  set(OPENDDS_SAFETY_PROFILE_INCLUDES "${DDS_ROOT}/dds/CORBA")
  set(OPENDDS_SAFETY_PROFILE_CORBA_LIB OpenDDS_Corba)

  if(OPENDDS_SAFETY_PROFILE EQUAL "EXTENDED")
    list(APPEND OPENDDS_SAFETY_PROFILE_MACRO ACE_HAS_ALLOC_HOOKS)
  endif()
endif()

if(${OPENDDS_SECURITY})
  set(OPENDDS_SECURITY_MACRO    "OPENDDS_SECURITY")
  set(OPENDDS_SECURITY_OPTION "-DOPENDDS_SECURITY")
  set(OPENDDS_SECURITY_LIB      "OpenDDS_Security")
endif()

if(${OPENDDS_RAPIDJSON})
  set(OPENDDS_RAPIDJSON_MACRO   "OPENDDS_RAPIDJSON")
  set(OPENDDS_RAPIDJSON_INCLUDE ${RapidJSON_INCLUDE_DIRS})
endif()

add_subdirectory(dds)
add_subdirectory(tests/DCPS/SequenceIterator)
add_subdirectory(tests/DCPS/HelloWorld)
add_subdirectory(tests/stress-tests)
add_subdirectory(tests/unit-tests)

if(${OPENDDS_SECURITY})
  add_subdirectory(tests/security/CheckInstance)
  add_subdirectory(tests/security/ConcurrentAuthLimit)
  add_subdirectory(tests/security/IDL_Serialization)
  add_subdirectory(tests/security/SingleParticipantWithSecurity)
  add_subdirectory(tests/security/TopicWithoutPermissions)
endif()
