# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#
# find_package for TAO. See OpenDDSConfig.cmake for OpenDDS.

cmake_minimum_required(VERSION 3.3.2)

if(OpenDDS-TAO_FOUND)
  return()
endif()
set(OpenDDS-TAO_FOUND FALSE)

find_package(OpenDDS-ACE REQUIRED)

set(_opendds_tao_required_deps TAO_LIBRARY TAO_IDL)

find_program(TAO_IDL NAMES tao_idl HINTS "${TAO_BIN_DIR}")

set(_opendds_tao_libs
  TAO
  TAO_IDL_FE
  TAO_AnyTypeCode
  TAO_BiDirGIOP
  TAO_CodecFactory
  TAO_IORManip
  TAO_IORTable
  TAO_ImR_Client
  TAO_PI
  TAO_PortableServer
  TAO_Svc_Utils
  TAO_Valuetype
)

if(OPENDDS_STATIC)
  list(APPEND TAO_COMPILE_DEFINITIONS TAO_AS_STATIC_LIBS)
endif()

if(OPENDDS_TAO_CORBA_E_COMPACT)
  list(APPEND TAO_COMPILE_DEFINITIONS CORBA_E_COMPACT)
endif()

if(OPENDDS_TAO_CORBA_E_MICRO)
  list(APPEND TAO_COMPILE_DEFINITIONS CORBA_E_MICRO)
endif()

if(OPENDDS_TAO_MINIMUM_CORBA)
  list(APPEND TAO_COMPILE_DEFINITIONS TAO_HAS_MINIMUM_CORBA=1)
endif()

set(TAO_DEPS ACE::ACE)
set(TAO_IDL_FE_DEPS ACE::ACE)
set(TAO_ANYTYPECODE_DEPS TAO::TAO)
set(TAO_BIDIRGIOP_DEPS TAO::TAO TAO::PI)
set(TAO_CODECFACTORY_DEPS TAO::TAO TAO::AnyTypeCode)
set(TAO_IORMANIP_DEPS TAO::TAO TAO::AnyTypeCode)
set(TAO_IORTABLE_DEPS TAO::TAO)
set(TAO_IMR_CLIENT_DEPS TAO::TAO TAO::PortableServer TAO::IORManip)
set(TAO_PI_DEPS TAO::TAO TAO::CodecFactory)
set(TAO_PORTABLESERVER_DEPS TAO::TAO TAO::AnyTypeCode)
set(TAO_SVC_UTILS_DEPS TAO::PortableServer TAO::AnyTypeCode)
set(TAO_VALUETYPE_DEPS TAO::TAO TAO::AnyTypeCode)

set(TAO_IDL_FE_INCLUDE_DIRS
  "${TAO_INCLUDE_DIRS}"
  # These only work with source tree right now
  "${TAO_ROOT}/TAO_IDL/include"
  "${TAO_ROOT}/TAO_IDL/be_include"
)

_opendds_find_our_libraries("TAO" "${_opendds_tao_libs}")
_opendds_found_required_deps(OpenDDS-TAO_FOUND "${_opendds_tao_required_deps}")
if(OpenDDS-TAO_FOUND)
  _opendds_add_target_binary(tao_idl "${TAO_IDL}")
  _opendds_add_library_group("TAO" "${_opendds_tao_libs}" TRUE)

  include("${CMAKE_CURRENT_LIST_DIR}/opendds_target_sources.cmake")
endif()
