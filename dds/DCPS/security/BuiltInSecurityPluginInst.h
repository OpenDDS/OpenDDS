/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BUILTIN_SECURITY_INST_H
#define OPENDDS_DCPS_BUILTIN_SECURITY_INST_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/security/framework/SecurityPluginInst.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

/**
 * @class BuiltInSecurityPluginInst
 *
 * @brief Factory object to create interfaces for the BuiltIn plugin.

 */

class OpenDDS_Dcps_Export BuiltInSecurityPluginInst : public OpenDDS::DCPS::SecurityPluginInst {
public:

	BuiltInSecurityPluginInst();
	~BuiltInSecurityPluginInst();

  virtual OpenDDS::DCPS::Authentication_rch create_authentication();
  virtual OpenDDS::DCPS::AccessControl_rch create_access_control();
  virtual OpenDDS::DCPS::CryptoKeyExchange_rch create_crypto_key_exchange();
  virtual OpenDDS::DCPS::CryptoKeyFactory_rch create_crypto_key_factory();
  virtual OpenDDS::DCPS::CryptoTransform_rch create_crypto_transform();

  virtual void shutdown();

private:

  BuiltInSecurityPluginInst(const BuiltInSecurityPluginInst& right);
  BuiltInSecurityPluginInst& operator=(const BuiltInSecurityPluginInst& right);
};


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
