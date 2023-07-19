cmake_minimum_required(VERSION 3.23)
project(opendds_security CXX)

set(deps OpenDDS::Rtps ACE::XML_Utils)
find_package(OpenDDS REQUIRED NO_DEFAULTS ${deps} security=TRUE)
include(opendds_build_helpers)

add_library(OpenDDS_Security
  AccessControl/Governance.cpp
  AccessControl/LocalAccessCredentialData.cpp
  AccessControl/Permissions.cpp
  AccessControl/XmlUtils.cpp
  AccessControlBuiltInImpl.cpp
  Authentication/LocalAuthCredentialData.cpp
  AuthenticationBuiltInImpl.cpp
  BuiltInPluginLoader.cpp
  BuiltInPlugins.cpp
  BuiltInSecurityPluginInst.cpp
  CommonUtilities.cpp
  CryptoBuiltInImpl.cpp
  SSL/Certificate.cpp
  SSL/DiffieHellman.cpp
  SSL/PrivateKey.cpp
  SSL/SignedDocument.cpp
  SSL/SubjectName.cpp
  SSL/Utils.cpp
  TokenReader.cpp
  TokenWriter.cpp
  UtilityImpl.cpp
)
target_sources(OpenDDS_Security
  PUBLIC FILE_SET HEADERS BASE_DIRS "${OPENDDS_SOURCE_DIR}" FILES
    AccessControl/DomainIdSet.h
    AccessControl/Governance.h
    AccessControl/LocalAccessCredentialData.h
    AccessControl/Permissions.h
    AccessControl/XmlUtils.h
    AccessControlBuiltInImpl.h
    Authentication/LocalAuthCredentialData.h
    AuthenticationBuiltInImpl.h
    BuiltInPluginLoader.h
    BuiltInPlugins.h
    BuiltInSecurityPluginInst.h
    CommonUtilities.h
    CryptoBuiltInImpl.h
    OpenDDS_Security_Export.h
    OpenSSL_init.h
    OpenSSL_legacy.h
    SSL/Certificate.h
    SSL/DiffieHellman.h
    SSL/Err.h
    SSL/PrivateKey.h
    SSL/SignedDocument.h
    SSL/SubjectName.h
    SSL/Utils.h
    TokenReader.h
    TokenWriter.h
    Utility.h
    UtilityImpl.h
    framework/HandleRegistry.h
    framework/Properties.h
    framework/SecurityConfig.h
    framework/SecurityConfigPropertyList.h
    framework/SecurityConfig_rch.h
    framework/SecurityPluginInst.h
    framework/SecurityPluginInst_rch.h
    framework/SecurityRegistry.h
)
_opendds_library(OpenDDS_Security)
target_link_libraries(OpenDDS_Security PUBLIC ${deps} PRIVATE OpenSSL::Crypto)

opendds_target_sources(OpenDDS_Security
  PUBLIC
    CryptoBuiltIn.idl
  INCLUDE_BASE "${OPENDDS_SOURCE_DIR}"
  USE_EXPORT "dds/DCPS/security/OpenDDS_Security_Export.h" OpenDDS_Security_Export
)
opendds_install_interface_files(OpenDDS_Security INCLUDE_BASE "${OPENDDS_SOURCE_DIR}")
