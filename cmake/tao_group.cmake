# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

cmake_minimum_required(VERSION 3.3...3.27)

if(_OPENDDS_TAO_GROUP_CMAKE)
  return()
endif()
set(_OPENDDS_TAO_GROUP_CMAKE TRUE)

include("${CMAKE_CURRENT_LIST_DIR}/import_common.cmake")

_opendds_group(TAO DEFAULT_REQUIRED TAO::TAO TAO::tao_idl)

_opendds_group_lib(TAO
  DEPENDS ACE::ACE
)
_opendds_group_lib(IDL_FE
  MPC_NAME TAO_IDL_FE
  DEPENDS ACE::ACE
)
set(TAO_IDL_FE_INCLUDE_DIRS
  "${TAO_INCLUDE_DIRS}"
  # TODO: These only work with source tree right now
  "${TAO_ROOT}/TAO_IDL/include"
  "${TAO_ROOT}/TAO_IDL/be_include"
)
_opendds_group_lib(AnyTypeCode DEPENDS TAO::TAO)
_opendds_group_lib(BiDirGIOP
  MPC_NAME BiDir_GIOP
  DEPENDS TAO::TAO TAO::PI
)
_opendds_group_lib(CodecFactory DEPENDS TAO::TAO TAO::AnyTypeCode)
_opendds_group_lib(IORManip
  MPC_NAME IORManipulation
  DEPENDS TAO::TAO TAO::AnyTypeCode
)
_opendds_group_lib(IORTable DEPENDS TAO::TAO)
_opendds_group_lib(ImR_Client DEPENDS TAO::TAO TAO::PortableServer TAO::IORManip)
_opendds_group_lib(PI DEPENDS TAO::TAO TAO::CodecFactory)
_opendds_group_lib(PortableServer DEPENDS TAO::TAO TAO::AnyTypeCode)
_opendds_group_lib(Svc_Utils DEPENDS TAO::PortableServer TAO::AnyTypeCode)
_opendds_group_lib(Valuetype DEPENDS TAO::TAO TAO::AnyTypeCode)

_opendds_group_exe(tao_idl
  MPC_NAME TAO_IDL_EXE
  DEPENDS ACE::ace_gperf
  HOST_TOOL
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
