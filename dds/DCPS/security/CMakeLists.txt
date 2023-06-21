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
_opendds_library(OpenDDS_Security)
target_link_libraries(OpenDDS_Security PUBLIC ${deps} PRIVATE OpenSSL::Crypto)
opendds_target_sources(OpenDDS_Security
  PUBLIC
    CryptoBuiltIn.idl
  INCLUDE_BASE "${OPENDDS_SOURCE_ROOT}"
  USE_EXPORT "dds/DCPS/security/OpenDDS_Security_Export.h" OpenDDS_Security_Export
)
